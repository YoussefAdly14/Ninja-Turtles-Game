#include "scene/Cutscene.h"
#include "scene/Timeline.h"
#include "scene/Environment.h"
#include "game/LaneManager.h"
#include "models/TurtleModels.h"
#include "models/VillainModels.h"
#include "models/PropModels.h"
#include "models/LairModel.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "ui/Text.h"
#include "Config.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <string>
#include <iostream>

// ================= shot list (seconds, CUTSCENE_DURATION = 16) =================
// 1   0.0 -  3.0  establishing: camera glides left through the sewer to the Lair, the canister glows,
//                 Donatello plants his shield; Leo + Raph step out of the Lair door
// 2   3.0 -  6.0  the Lair: Leo + Raph guard the canister, Leo says why it matters
// 3   6.0 -  9.0  invasion: rumble + falling grit, the manhole cover bursts, two Foot Soldiers drop in,
//                 Rocksteady + Bebop stomp out of the dust
// 4   9.0 - 12.0  standoff: both sides advance in their lanes, Leo flourishes his katanas,
//                 Don slams his shield, Bebop's warning shot bounces off it
// 5  12.0 - 14.0  clash: a Foot Soldier throws a shuriken, Raph's sai knocks it out of the air
// 6  14.0 - 16.0  transition: camera pulls back to the game framing, villains fall back, iris closes on
//                 the Lair and opens again on the empty playfield exactly as the game starts

// ---- whole cutscene ----
const float FADE_IN_END = 0.6f;
const float LETTERBOX = 50.0f;                  // cinematic bars, the subtitles sit in the bottom one
const float SUB_SCALE = 0.55f;

// ---- shot 1 + 2: the Lair ----
const glm::vec2 CANISTER_POS(215.0f, 0.0f);     // x, height above lane 2's floor
const int   CANISTER_LANE = 2;
const float LEO_APPEAR = 2.0f;
const float RAPH_APPEAR = 2.3f;
const float DON_SLAM_DURATION = 0.5f;
const float DON_SLAMS[] = { 0.8f, 2.0f, 4.4f, 10.7f };   // the last one is the standoff slam

// ---- shot 3: invasion ----
const float RUMBLE_START = 6.0f, RUMBLE_END = 7.5f;
const float GRIT_START = 6.0f, GRIT_SPREAD = 1.6f;
const glm::vec2 MANHOLE(1230.0f, PLAYFIELD_TOP);  // the hole in the ceiling
const float COVER_BURST = 6.9f, COVER_LAND = 7.45f;
const float DROP_TIME = 0.4f;
const float FOOT_A_DROP = 7.15f, FOOT_B_DROP = 7.45f;
const float VILLAIN_DUST_START = 7.5f, VILLAIN_DUST_END = 9.6f;
const float ROCK_APPEAR = 7.8f, BEBOP_APPEAR = 8.0f;
const float ROCK_PUNCH = 8.6f;

// ---- shot 4: standoff ----
const float ADVANCE_START = 9.0f, ADVANCE_END = 10.0f;
const float LEO_FLOURISH_1 = 10.0f, LEO_FLOURISH_2 = 10.36f;
const float BEBOP_SHOT = 11.0f;                 // attack starts, the bolt leaves at STRIKE_POINT
const float BOLT_HIT = 11.6f;

// ---- shot 5: clash ----
const float FOOT_THROW = 12.3f;
const float RAPH_THROW = 12.55f;
const float CLASH_TIME = 13.0f;
const glm::vec2 CLASH_POINT(770.0f, 517.0f);
const float SAI_STUCK = 13.35f, SHURIKEN_STUCK = 13.75f;
const glm::vec2 SAI_STUCK_POINT(905.0f, 444.0f);  // in lane 1's floor, at the Foot Soldier's feet

// ---- shot 6: transition ----
const float PULLBACK_START = 13.9f, PULLBACK_END = 14.9f;
const float IRIS_CLOSE_START = 14.8f, IRIS_CLOSED = 15.3f, IRIS_OPEN_START = 15.45f;
const float IRIS_MAX_RADIUS = 1750.0f;          // reaches every corner from the Lair door

// ================= camera (focus point + zoom) =================
static const Track<glm::vec2> camFocus = {
    { 0.0f,  { 1067.0f, 380.0f } },
    { 3.0f,  {  470.0f, 320.0f }, Ease::InOut },   // glide to the Lair
    { 6.0f,  {  490.0f, 325.0f } },                // slow push in
    { 6.2f,  {  490.0f, 325.0f } },
    { 7.0f,  {  930.0f, 470.0f }, Ease::InOut },   // whip to the manhole as it bursts
    { 7.8f,  {  930.0f, 470.0f } },
    { 8.6f,  {  945.0f, 425.0f }, Ease::InOut },   // tilt down for Rocksteady + Bebop
    { 9.0f,  {  950.0f, 425.0f } },
    { 9.6f,  {  800.0f, 405.0f }, Ease::InOut },   // standoff: all four lanes in frame
    { 12.0f, {  800.0f, 405.0f } },
    { 12.4f, {  760.0f, 480.0f }, Ease::InOut },   // close on lane 1 for the sai vs shuriken
    { PULLBACK_START, { 760.0f, 480.0f } },
    { PULLBACK_END,   { 800.0f, 450.0f }, Ease::InOut },
};
static const Track<float> camZoom = {
    { 0.0f,  1.5f },
    { 3.0f,  1.55f, Ease::InOut },
    { 6.0f,  1.58f },
    { 6.2f,  1.58f },
    { 7.0f,  1.15f, Ease::InOut },
    { 9.0f,  1.15f },
    { 9.6f,  1.1f, Ease::InOut },
    { 12.0f, 1.1f },
    { 12.4f, 1.5f, Ease::InOut },
    { PULLBACK_START, 1.5f },
    { PULLBACK_END,   1.0f, Ease::InOut },        // = the in-game framing
};

// ================= actors =================
// path = (x, height above the lane floor). Hidden before `appear`.
enum ActorId { LEO, RAPH, DON, FOOT_A, FOOT_B, ROCK, BEBOP, ACTOR_COUNT };

struct Actor
{
    int lane;
    float appear;
    float walkRate;            // walkCycle per second while the actor moves
    Track<glm::vec2> path;
};

static const Actor actors[ACTOR_COUNT] = {
    // LEO: steps out of the Lair door, guards the canister, advances for the standoff
    { 2, LEO_APPEAR, WALK_CYCLE_SPEED, {
        { LEO_APPEAR, { 110.0f, 0.0f } },
        { 3.4f, { 330.0f, 0.0f } },
        { ADVANCE_START, { 330.0f, 0.0f } },
        { ADVANCE_END, { 600.0f, 0.0f }, Ease::InOut },
        { PULLBACK_START, { 600.0f, 0.0f } },
        { PULLBACK_END, { 540.0f, 0.0f }, Ease::InOut } } },
    // RAPH
    { 1, RAPH_APPEAR, WALK_CYCLE_SPEED, {
        { RAPH_APPEAR, { 110.0f, 0.0f } },
        { 3.8f, { 300.0f, 0.0f } },
        { ADVANCE_START, { 300.0f, 0.0f } },
        { ADVANCE_END, { 520.0f, 0.0f }, Ease::InOut },
        { PULLBACK_START, { 520.0f, 0.0f } },
        { PULLBACK_END, { 470.0f, 0.0f }, Ease::InOut } } },
    // DON: the blocker never moves (same as in the game)
    { 3, 0.0f, 0.0f, { { 0.0f, { 235.0f, 0.0f } } } },
    // FOOT_A: drops from the manhole into lane 1
    { 1, FOOT_A_DROP, WALK_CYCLE_SPEED, {
        { FOOT_A_DROP, { 1240.0f, MANHOLE.y + 20.0f - laneFloorY(1) } },
        { FOOT_A_DROP + DROP_TIME, { 1260.0f, 0.0f } },
        { ADVANCE_START, { 1260.0f, 0.0f } },
        { ADVANCE_END, { 1000.0f, 0.0f }, Ease::InOut },
        { SAI_STUCK, { 1000.0f, 0.0f } },
        { SAI_STUCK + 0.45f, { 1060.0f, 0.0f }, Ease::InOut },   // flinches back from the sai
        { PULLBACK_START, { 1060.0f, 0.0f } },
        { PULLBACK_END, { 1300.0f, 0.0f } } } },
    // FOOT_B: drops into lane 0
    { 0, FOOT_B_DROP, WALK_CYCLE_SPEED, {
        { FOOT_B_DROP, { 1215.0f, MANHOLE.y + 20.0f - laneFloorY(0) } },
        { FOOT_B_DROP + DROP_TIME * 0.6f, { 1180.0f, 0.0f } },
        { ADVANCE_START, { 1180.0f, 0.0f } },
        { ADVANCE_END, { 1150.0f, 0.0f }, Ease::InOut },
        { PULLBACK_START, { 1150.0f, 0.0f } },
        { PULLBACK_END, { 1350.0f, 0.0f } } } },
    // ROCK: stomps out of the dust in lane 2, fast like in the game
    { 2, ROCK_APPEAR, WALK_CYCLE_SPEED * 1.6f, {
        { ROCK_APPEAR, { 1700.0f, 0.0f } },
        { 9.0f, { 1300.0f, 0.0f } },
        { 9.1f, { 1300.0f, 0.0f } },
        { 10.0f, { 1040.0f, 0.0f }, Ease::InOut },
        { PULLBACK_START, { 1040.0f, 0.0f } },
        { PULLBACK_END, { 1250.0f, 0.0f } } } },
    // BEBOP: lane 3, across from Donatello
    { 3, BEBOP_APPEAR, WALK_CYCLE_SPEED, {
        { BEBOP_APPEAR, { 1700.0f, 0.0f } },
        { 9.2f, { 1340.0f, 0.0f } },
        { 10.2f, { 1000.0f, 0.0f }, Ease::InOut },
        { PULLBACK_START, { 1000.0f, 0.0f } },
        { PULLBACK_END, { 1280.0f, 0.0f } } } },
};

// ================= dialogue (subtitles support the action, they never carry it alone) =================
struct Line
{
    float start, end;
    int who;                   // ActorId, the speaker bobs while the line shows (req 33)
    const char* text;
};

static const Line lines[] = {
    { 3.3f,  5.9f,  LEO,   "The last canister of mutagen. In the wrong hands, it makes monsters." },
    { 6.15f, 7.5f,  RAPH,  "Uh-oh. We got company." },
    { 7.7f,  9.4f,  ROCK,  "Shredder wants that mutagen. Smash everything!" },
    { 9.5f,  10.9f, LEO,   "You'll have to get through us first." },
    { 11.0f, 12.2f, BEBOP, "Heh heh... too easy!" },
    { 13.3f, 14.6f, RAPH,  "Next one's not a warning." },
    { 14.7f, 15.7f, LEO,   "Turtles... defend the Lair!" },
};

static const char* speakerName(int who)
{
    switch (who) {
    case LEO:   return "LEONARDO";
    case RAPH:  return "RAPHAEL";
    case DON:   return "DONATELLO";
    case ROCK:  return "ROCKSTEADY";
    case BEBOP: return "BEBOP";
    default:    return "FOOT SOLDIER";
    }
}

static glm::vec4 speakerColor(int who)   // the mask colours, brightened to read on the black bar
{
    switch (who) {
    case LEO:   return hexColor(0x5A93F0);
    case RAPH:  return hexColor(0xF04A4A);
    case DON:   return hexColor(0xB27BE0);
    default:    return hexColor(0xB8B8B8);   // villains: grey family
    }
}

// ================= small helpers =================

// 0 at rest, 0..1 while a swing/throw/shot that started at `start` plays (the Pose::attack contract)
static float attackAt(float t, float start, float duration = ATTACK_ANIM_DURATION)
{
    if (t < start || t >= start + duration) return 0.0f;
    return (t - start) / duration;
}

// 1 at `start`, fading to 0 over `length` (flinches, camera jolts)
static float pulseAt(float t, float start, float length)
{
    if (t < start || t >= start + length) return 0.0f;
    return 1.0f - (t - start) / length;
}

// cheap repeatable "random" 0..1 for the grit (same every run, so draw() stays const)
static float hash01(int i)
{
    float s = sinf(i * 12.9898f) * 43758.5453f;
    return s - floorf(s);
}

static float floorOf(int lane) { return laneFloorY(lane); }

static bool talking(int who, float t)
{
    for (const Line& l : lines)
        if (l.who == who && t >= l.start && t < l.end) return true;
    return false;
}

static float donSlam(float t)
{
    float a = 0.0f;
    for (float s : DON_SLAMS) a += attackAt(t, s, DON_SLAM_DURATION);
    return a;
}

// ================= camera =================
// 2D version of the learnopengl Camera view matrix: move the focus point to the screen centre and zoom
// around it (Lab 4 view matrix idea). The focus is clamped so the camera never shows outside the world.
static glm::mat4 cameraView(float t, glm::vec2 shake)
{
    float zoom = sample(camZoom, t);
    glm::vec2 focus = sample(camFocus, t);
    float halfW = WORLD_WIDTH * 0.5f / zoom, halfH = WORLD_HEIGHT * 0.5f / zoom;
    const float margin = 10.0f / zoom;   // room for the shake
    focus.x = fmaxf(halfW + margin, fminf(WORLD_WIDTH - halfW - margin, focus.x));
    focus.y = fmaxf(halfH + margin, fminf(WORLD_HEIGHT - halfH - margin, focus.y));
    if (zoom <= 1.0f) focus = glm::vec2(WORLD_WIDTH * 0.5f, WORLD_HEIGHT * 0.5f);

    glm::mat4 view = translate2D(glm::mat4(1.0f), WORLD_WIDTH * 0.5f + shake.x, WORLD_HEIGHT * 0.5f + shake.y);
    view = scale2D(view, zoom, zoom);
    return translate2D(view, -focus.x, -focus.y);
}

static glm::vec2 cameraShake(float t)
{
    float amount = 7.0f * fadeWindow(t, RUMBLE_START, RUMBLE_END, 0.2f);
    amount += 6.0f * pulseAt(t, COVER_LAND, 0.3f);
    // Rocksteady's footsteps while he stomps in
    float rockWalk = segment(t, ROCK_APPEAR, 9.0f);
    if (rockWalk > 0.0f && rockWalk < 1.0f) amount += 3.0f * fabsf(sinf(t * WALK_CYCLE_SPEED * 1.6f));
    amount += 5.0f * pulseAt(t, DON_SLAMS[3] + DON_SLAM_DURATION * 0.5f, 0.25f);
    return glm::vec2(sinf(t * 57.0f), cosf(t * 43.0f)) * amount;
}

// ================= effects + set pieces (all multi-part, Renderer shapes + Transform.h) =================

// star burst: thin spikes + hot core, k = 0..1 progress
static void drawSpark(glm::vec2 c, float k, float size, glm::vec4 color)
{
    if (k <= 0.0f || k >= 1.0f) return;
    float alpha = 1.0f - k;
    float len = size * (0.5f + k);
    glm::mat4 base = translate2D(glm::mat4(1.0f), c.x, c.y);
    for (int i = 0; i < 8; ++i) {
        glm::mat4 spike = rotate2D(base, i * 45.0f + k * 30.0f);
        float l = (i % 2 == 0) ? len : len * 0.6f;
        Renderer::draw(Shape::Triangle, part(spike, 0.0f, l * 0.5f, size * 0.12f, l), withAlpha(color, alpha));
    }
    Renderer::drawCircle(c, size * 0.25f * (1.0f - k * 0.5f), withAlpha(WHITE, alpha));
}

// dust cloud: a cluster of puffs that grows and fades
static void drawDustCloud(glm::vec2 c, float k, float size)
{
    if (k <= 0.0f || k >= 1.0f) return;
    const glm::vec4 dust = hexColor(0x8A8270);
    float alpha = 0.55f * (1.0f - k) * segment(k, 0.0f, 0.1f);   // quick puff in, slow fade out
    float grow = 0.6f + 0.8f * k;
    const glm::vec2 offsets[] = { { 0.0f, 0.0f }, { -0.5f, -0.15f }, { 0.5f, -0.1f }, { -0.25f, 0.35f }, { 0.3f, 0.3f }, { 0.0f, -0.3f } };
    const float radii[] = { 0.5f, 0.38f, 0.42f, 0.34f, 0.3f, 0.36f };
    for (int i = 0; i < 6; ++i)
        Renderer::drawCircle(c + offsets[i] * size * grow, radii[i] * size * grow, withAlpha(dust, alpha));
}

// iris wipe: the screen is black except a circle. Each quad covers the half-plane outside one tangent
// of the circle, 36 of them leave a round hole (rotated quads, learnopengl Transformations).
static void drawIris(glm::vec2 c, float radius)
{
    const int sides = 36;
    const float big = 4000.0f;
    for (int i = 0; i < sides; ++i) {
        float angle = i * 360.0f / sides;
        glm::mat4 m = rotate2D(translate2D(glm::mat4(1.0f), c.x, c.y), angle);
        Renderer::draw(Shape::Quad, part(m, radius + big * 0.5f, 0.0f, big, big), BLACK);
    }
}

// The last mutagen canister on its pedestal: glass tube, glowing ooze, caps, bands, hazard mark, bubbles.
static void drawCanister(glm::vec2 feet, float t)
{
    const glm::vec4 metal = hexColor(0x8C969C);
    const glm::vec4 metalDark = hexColor(0x4B5357);
    const glm::vec4 glass = hexColor(0xBFE8D0, 0.35f);
    float pulse = 0.5f + 0.5f * sinf(t * 3.0f);
    glm::mat4 base = translate2D(glm::mat4(1.0f), feet.x, feet.y);

    // glow halo (drawn first so everything sits in it)
    Renderer::draw(Shape::Circle, part(base, 0.0f, 90.0f, 230.0f + 30.0f * pulse, 230.0f + 30.0f * pulse), withAlpha(MUTAGEN_GLOW, 0.07f));
    Renderer::draw(Shape::Circle, part(base, 0.0f, 90.0f, 150.0f + 20.0f * pulse, 150.0f + 20.0f * pulse), withAlpha(MUTAGEN_GLOW, 0.12f));

    // pedestal: foot, column with rivets, top plate
    Renderer::draw(Shape::Quad, part(base, 0.0f, 5.0f, 60.0f, 10.0f), metalDark);
    Renderer::draw(Shape::Quad, part(base, 0.0f, 26.0f, 34.0f, 32.0f), metal);
    Renderer::draw(Shape::Quad, part(base, -10.0f, 26.0f, 4.0f, 32.0f), metalDark);
    Renderer::drawCircle({ feet.x + 9.0f, feet.y + 34.0f }, 2.5f, metalDark);
    Renderer::drawCircle({ feet.x + 9.0f, feet.y + 18.0f }, 2.5f, metalDark);
    Renderer::draw(Shape::Quad, part(base, 0.0f, 46.0f, 54.0f, 8.0f), metalDark);

    // canister: bottom cap, glass, ooze, bands, top cap + handle
    glm::mat4 can = translate2D(base, 0.0f, 50.0f);
    Renderer::draw(Shape::Quad, part(can, 0.0f, 5.0f, 44.0f, 10.0f), metal);
    float fill = 52.0f;
    Renderer::draw(Shape::Quad, part(can, 0.0f, 10.0f + fill * 0.5f, 30.0f, fill), withAlpha(MUTAGEN_GLOW, 0.75f + 0.25f * pulse));
    Renderer::draw(Shape::Semicircle, part(can, 0.0f, 10.0f + fill, 30.0f, 8.0f + 3.0f * pulse), withAlpha(MUTAGEN_GLOW, 0.9f));
    Renderer::draw(Shape::Quad, part(can, 0.0f, 40.0f, 36.0f, 60.0f), glass);
    Renderer::draw(Shape::Quad, part(can, -11.0f, 40.0f, 4.0f, 54.0f), withAlpha(WHITE, 0.45f));   // shine
    Renderer::draw(Shape::Quad, part(can, 0.0f, 24.0f, 38.0f, 4.0f), metalDark);
    Renderer::draw(Shape::Quad, part(can, 0.0f, 56.0f, 38.0f, 4.0f), metalDark);
    Renderer::draw(Shape::Quad, part(can, 0.0f, 75.0f, 44.0f, 10.0f), metal);
    Renderer::draw(Shape::Ring, part(can, 0.0f, 86.0f, 20.0f, 16.0f), metalDark);
    // hazard mark: three blades around a dot
    glm::mat4 mark = translate2D(can, 2.0f, 40.0f);
    for (int i = 0; i < 3; ++i)
        Renderer::draw(Shape::Triangle, part(rotate2D(mark, i * 120.0f + 180.0f), 0.0f, 6.0f, 8.0f, 8.0f), hexColor(0x1E2A12));
    Renderer::drawCircle({ feet.x + 2.0f, feet.y + 90.0f }, 2.5f, hexColor(0x1E2A12));

    // bubbles rising through the ooze
    for (int i = 0; i < 4; ++i) {
        float k = fmodf(t * 0.8f + i * 0.25f, 1.0f);
        float bx = -8.0f + 6.0f * i + 2.0f * sinf(t * 4.0f + i);
        Renderer::draw(Shape::Circle, part(can, bx, 12.0f + k * (fill - 4.0f), 4.0f, 4.0f), withAlpha(hexColor(0xE8FFD0), 0.8f * (1.0f - k)));
    }
}

// Manhole in the ceiling: shaft with a ladder, concrete collar, street light pouring in once the cover is gone,
// and the cover itself (a disc that tumbles: squashing its height fakes the flip).
static void drawManholeCover(glm::mat4 m, float flipDegrees)
{
    const glm::vec4 iron = hexColor(0x3E4448);
    const glm::vec4 ironLight = hexColor(0x5D666B);
    float squash = fmaxf(0.12f, fabsf(cosf(glm::radians(flipDegrees))));
    glm::mat4 disc = scale2D(m, 1.0f, squash);
    Renderer::draw(Shape::Circle, part(disc, 0.0f, 0.0f, 110.0f, 110.0f), iron);
    Renderer::draw(Shape::Ring, part(disc, 0.0f, 0.0f, 110.0f, 110.0f), ironLight);
    for (int i = -2; i <= 2; ++i) {   // grip pattern
        Renderer::draw(Shape::Quad, part(disc, i * 16.0f, 0.0f, 6.0f, 64.0f), ironLight);
        Renderer::draw(Shape::Quad, part(disc, 0.0f, i * 16.0f, 64.0f, 6.0f), ironLight);
    }
    Renderer::draw(Shape::Circle, part(disc, 0.0f, 0.0f, 18.0f, 18.0f), iron);
}

static void drawManhole(float t)
{
    const glm::vec4 concrete = hexColor(0x6F7470);
    const glm::vec4 rust = hexColor(0x7A4A2A);
    float open = segment(t, COVER_BURST, COVER_BURST + 0.2f);
    glm::mat4 base = translate2D(glm::mat4(1.0f), MANHOLE.x, MANHOLE.y);

    // light cone from the street (behind everything that falls through it)
    if (open > 0.0f)
        Renderer::draw(Shape::Triangle, part(base, 0.0f, -230.0f, 320.0f, 520.0f), withAlpha(hexColor(0xFFE9A8), 0.12f * open));

    // shaft + ladder
    Renderer::draw(Shape::Quad, part(base, 0.0f, 70.0f, 130.0f, 140.0f), hexColor(0x121715));
    if (open > 0.0f)
        Renderer::draw(Shape::Quad, part(base, 0.0f, 110.0f, 110.0f, 60.0f), withAlpha(hexColor(0xFFE9A8), 0.55f * open));
    Renderer::draw(Shape::Quad, part(base, -40.0f, 70.0f, 5.0f, 140.0f), rust);
    Renderer::draw(Shape::Quad, part(base, 40.0f, 70.0f, 5.0f, 140.0f), rust);
    for (int i = 0; i < 5; ++i)
        Renderer::draw(Shape::Quad, part(base, 0.0f, 12.0f + i * 26.0f, 80.0f, 4.0f), rust);
    // concrete collar
    Renderer::draw(Shape::Quad, part(base, -78.0f, 8.0f, 28.0f, 26.0f), concrete);
    Renderer::draw(Shape::Quad, part(base, 78.0f, 8.0f, 28.0f, 26.0f), concrete);
    Renderer::draw(Shape::Quad, part(base, 0.0f, -4.0f, 190.0f, 6.0f), hexColor(0x50554F));

    // the cover: plugs the hole, then gets kicked in and tumbles down onto lane 0
    static const Track<glm::vec2> coverPath = {
        { COVER_BURST, { MANHOLE.x, MANHOLE.y + 6.0f } },
        { COVER_BURST + 0.2f, { MANHOLE.x - 40.0f, MANHOLE.y - 40.0f } },
        { COVER_LAND, { MANHOLE.x - 120.0f, laneFloorY(0) + 7.0f } },
    };
    static const Track<float> coverSpin = { { COVER_BURST, 0.0f }, { COVER_LAND, 360.0f } };
    static const Track<float> coverFlip = { { COVER_BURST, 90.0f }, { COVER_LAND, 810.0f } };   // edge-on -> 2 flips -> edge-on
    glm::vec2 p = sample(coverPath, t);
    drawManholeCover(rotate2D(translate2D(glm::mat4(1.0f), p.x, p.y), sample(coverSpin, t)), sample(coverFlip, t));
}

// grit and pebbles shaken loose by the rumble, plus chunks from the manhole burst
static void drawGrit(float t)
{
    const glm::vec4 rock = hexColor(0x5B5448);
    const glm::vec4 speck = hexColor(0x9C9282);
    const float gravity = 900.0f;
    for (int i = 0; i < 40; ++i) {
        float start = GRIT_START + hash01(i + 50) * GRIT_SPREAD;
        float age = t - start;
        if (age < 0.0f) continue;
        float x = 220.0f + hash01(i) * 1340.0f;
        float y = PLAYFIELD_TOP + 10.0f - 0.5f * gravity * age * age;
        if (y < PLAYFIELD_BOTTOM) continue;
        float size = 4.0f + 5.0f * hash01(i + 99);
        Renderer::drawRect({ x, y }, { size, size }, rock, age * 400.0f + i * 30.0f);
        Renderer::drawRect({ x + 1.0f, y + 1.0f }, { size * 0.4f, size * 0.4f }, speck);
    }
    for (int i = 0; i < 14; ++i) {   // burst chunks
        float age = t - COVER_BURST;
        if (age < 0.0f || age > 1.2f) continue;
        float vx = (hash01(i + 200) - 0.5f) * 500.0f;
        float vy = -50.0f - hash01(i + 300) * 250.0f;
        glm::vec2 p(MANHOLE.x + vx * age, MANHOLE.y + vy * age - 0.5f * gravity * age * age);
        float size = 6.0f + 6.0f * hash01(i + 400);
        Renderer::drawRect(p, { size, size * 0.8f }, hexColor(0x6F7470), age * 500.0f + i * 40.0f);
        Renderer::drawRect(p + glm::vec2(1.5f, 1.5f), { size * 0.4f, size * 0.3f }, speck);
    }
}

// ================= actors =================

static glm::vec2 actorPos(const Actor& a, float t)
{
    glm::vec2 p = sample(a.path, t);
    return glm::vec2(p.x, floorOf(a.lane) + p.y);
}

static bool actorWalking(const Actor& a, float t)
{
    if (a.walkRate <= 0.0f) return false;
    glm::vec2 now = sample(a.path, t), before = sample(a.path, t - 0.05f);
    return fabsf(now.x - before.x) > 0.5f && fabsf(now.y - before.y) < 0.5f;   // falling isn't walking
}

static Pose actorPose(int id, float t)
{
    const Actor& a = actors[id];
    Pose pose;
    pose.time = t;
    if (actorWalking(a, t)) pose.walkCycle = t * a.walkRate;

    float rumbleFlinch = 0.6f * pulseAt(t, RUMBLE_START + 0.05f, 0.5f);
    switch (id) {
    case LEO:
        pose.attack = attackAt(t, LEO_FLOURISH_1) + attackAt(t, LEO_FLOURISH_2);
        pose.hitFlash = rumbleFlinch;
        break;
    case RAPH:
        pose.attack = attackAt(t, RAPH_THROW);
        pose.hitFlash = rumbleFlinch;
        break;
    case DON:
        pose.attack = donSlam(t);
        pose.hitFlash = pulseAt(t, BOLT_HIT, 0.3f) + rumbleFlinch * 0.5f;
        break;
    case FOOT_A:
    case FOOT_B: {
        float drop = (id == FOOT_A) ? FOOT_A_DROP : FOOT_B_DROP;
        float land = a.path[1].time;
        pose.inAir = t >= drop && t < land;
        pose.ducking = t >= land && t < land + 0.2f;   // landing crouch
        if (id == FOOT_A) {
            pose.attack = attackAt(t, FOOT_THROW);
            pose.hitFlash = pulseAt(t, SAI_STUCK, 0.4f);
        }
        break;
    }
    case ROCK:
        pose.attack = attackAt(t, ROCK_PUNCH);
        break;
    case BEBOP:
        pose.attack = attackAt(t, BEBOP_SHOT);
        break;
    }
    return pose;
}

static void drawActor(int id, float t)
{
    const Actor& a = actors[id];
    if (t < a.appear) return;
    glm::vec2 p = actorPos(a, t);
    if (talking(id, t)) p.y += fabsf(sinf(t * 9.0f)) * 4.0f;   // the speaker bobs while talking
    glm::mat4 base = translate2D(glm::mat4(1.0f), p.x, p.y);
    Pose pose = actorPose(id, t);

    switch (id) {
    case LEO:    TurtleModels::drawLeonardo(base, pose); break;
    case RAPH:   TurtleModels::drawRaphael(base, pose); break;
    case DON:    TurtleModels::drawDonatello(base, pose); break;
    case FOOT_A:
    case FOOT_B: VillainModels::drawFootSoldier(base, pose); break;
    case ROCK:   VillainModels::drawRocksteady(base, pose); break;
    case BEBOP:  VillainModels::drawBebop(base, pose); break;
    }
}

// ================= projectiles of the standoff + clash =================
static void drawProjectiles(float t)
{
    float strike = ATTACK_ANIM_DURATION * STRIKE_POINT;

    // Bebop's warning shot: LOW, straight into Donatello's shield
    float boltStart = BEBOP_SHOT + strike;
    if (t >= boltStart && t < BOLT_HIT) {
        glm::vec2 from(actorPos(actors[BEBOP], boltStart).x - 60.0f, floorOf(3) + LOW_SHOT_HEIGHT + 10.0f);
        glm::vec2 to(actorPos(actors[DON], t).x + 45.0f, from.y);
        glm::vec2 p = lerp(from, to, segment(t, boltStart, BOLT_HIT));
        PropModels::drawBlasterBolt(scale2D(translate2D(glm::mat4(1.0f), p.x, p.y), 1.4f, 1.4f), t);
    }
    drawSpark({ actorPos(actors[DON], t).x + 45.0f, floorOf(3) + LOW_SHOT_HEIGHT + 10.0f },
              segment(t, BOLT_HIT, BOLT_HIT + 0.35f), 40.0f, hexColor(0xFFB347));

    // Foot Soldier's shuriken (HIGH) vs Raphael's sai: they meet mid-lane, the sai knocks the star away
    float shurikenStart = FOOT_THROW + strike;
    const Track<glm::vec2> shurikenPath = {
        { shurikenStart, { 975.0f, floorOf(1) + 105.0f } },
        { CLASH_TIME, CLASH_POINT },
        { CLASH_TIME + 0.25f, { CLASH_POINT.x - 60.0f, CLASH_POINT.y + 80.0f }, Ease::InOut },   // knocked up...
        { SHURIKEN_STUCK, { 640.0f, floorOf(1) + 8.0f } },                                     // ...and down into the floor
    };
    const Track<float> shurikenSpin = { { shurikenStart, 0.0f }, { SHURIKEN_STUCK, 900.0f } };
    if (t >= shurikenStart) {
        glm::vec2 p = sample(shurikenPath, t);
        PropModels::drawShuriken(scale2D(translate2D(glm::mat4(1.0f), p.x, p.y), 1.5f, 1.5f), sample(shurikenSpin, t));
    }

    float saiStart = RAPH_THROW + strike;
    const Track<glm::vec2> saiPath = {
        { saiStart, { 555.0f, floorOf(1) + 72.0f } },
        { CLASH_TIME, CLASH_POINT },
        { SAI_STUCK, SAI_STUCK_POINT },   // sticks in the floor at the Foot Soldier's feet
    };
    const Track<float> saiSpin = { { saiStart, 0.0f }, { CLASH_TIME, 215.0f }, { SAI_STUCK, 420.0f } };   // ends blade down-right
    if (t >= saiStart) {
        glm::vec2 p = sample(saiPath, t);
        PropModels::drawSai(scale2D(translate2D(glm::mat4(1.0f), p.x, p.y), 1.5f, 1.5f), sample(saiSpin, t));
    }

    drawSpark(CLASH_POINT, segment(t, CLASH_TIME, CLASH_TIME + 0.4f), 70.0f, hexColor(0xFFF2B0));
    drawSpark(SAI_STUCK_POINT - glm::vec2(0.0f, 6.0f), segment(t, SAI_STUCK, SAI_STUCK + 0.25f), 26.0f, hexColor(0xFFF2B0));
}

// ================= Cutscene =================

void Cutscene::start()
{
    m_time = 0.0f;
    m_startWallTime = glfwGetTime();
    m_logged = false;
}

void Cutscene::update(float dt)
{
    m_time += dt;
#ifndef NDEBUG
    // req 31: prove the length stays inside the spec's 10-20 s
    if (finished() && !m_logged) {
        std::cout << "Cutscene: " << m_time << " s story time, "
                  << (glfwGetTime() - m_startWallTime) << " s wall time (spec: 10-20 s)\n";
        m_logged = true;
    }
#endif
}

bool Cutscene::finished() const
{
    return m_time >= CUTSCENE_DURATION;
}

void Cutscene::skip()
{
    m_time = CUTSCENE_DURATION;
}

void Cutscene::draw() const
{
    float t = m_time;
    LaneManager lanes;   // nothing hovered or selected: the walkways exactly as the game starts
    const glm::vec2 lairDoor(LAIR_WIDTH * 0.5f, (PLAYFIELD_BOTTOM + PLAYFIELD_TOP) * 0.5f);

    if (t < IRIS_CLOSED) {
        // ---- the story, seen through the animated camera ----
        glm::mat4 view = cameraView(t, cameraShake(t));
        Renderer::setView(view);

        Environment::drawBackground(t);
        Environment::drawProps(t);
        lanes.draw(t);
        LairModel::draw(t);
        drawManhole(t);

        // top lane first, like Game::renderWorld (lower lanes overlap the ones behind them)
        for (int lane = 0; lane < LANE_COUNT; ++lane) {
            if (lane == CANISTER_LANE) drawCanister({ CANISTER_POS.x, floorOf(lane) + CANISTER_POS.y }, t);
            for (int id = 0; id < ACTOR_COUNT; ++id)
                if (actors[id].lane == lane && id >= FOOT_A) drawActor(id, t);
            for (int id = 0; id < ACTOR_COUNT; ++id)
                if (actors[id].lane == lane && id < FOOT_A) drawActor(id, t);
        }

        drawProjectiles(t);

        // Don's slams kick up dust and sparks off the shield edge
        for (float s : DON_SLAMS) {
            float peak = s + DON_SLAM_DURATION * 0.5f;
            glm::vec2 shield(actorPos(actors[DON], t).x + 40.0f, floorOf(3) + 4.0f);
            drawDustCloud(shield + glm::vec2(10.0f, 10.0f), segment(t, peak, peak + 0.6f), 40.0f);
            drawSpark(shield, segment(t, peak, peak + 0.25f), 26.0f, hexColor(0xFFE08A));
        }
        // dust where the Foot Soldiers land and where Rocksteady + Bebop stomp in
        drawDustCloud({ actors[FOOT_A].path[1].value.x, floorOf(1) + 10.0f }, segment(t, FOOT_A_DROP + DROP_TIME, FOOT_A_DROP + DROP_TIME + 0.7f), 45.0f);
        drawDustCloud({ actors[FOOT_B].path[1].value.x, floorOf(0) + 10.0f }, segment(t, actors[FOOT_B].path[1].time, actors[FOOT_B].path[1].time + 0.7f), 45.0f);
        drawDustCloud({ 1110.0f, floorOf(0) + 10.0f }, segment(t, COVER_LAND, COVER_LAND + 0.6f), 40.0f);
        for (int lane = 2; lane <= 3; ++lane) {
            float k = segment(t, VILLAIN_DUST_START + (lane - 2) * 0.2f, VILLAIN_DUST_END);
            drawDustCloud({ 1540.0f, floorOf(lane) + 60.0f }, k, 150.0f);
            drawDustCloud({ 1440.0f, floorOf(lane) + 30.0f }, segment(k, 0.2f, 1.0f), 90.0f);
        }
        drawGrit(t);

        Renderer::setView(glm::mat4(1.0f));   // overlays below don't move with the camera

        // iris closes on the Lair door (where it is on screen right now)
        float close = segment(t, IRIS_CLOSE_START, IRIS_CLOSED);
        if (close > 0.0f) {
            glm::vec4 door = view * glm::vec4(lairDoor, 0.0f, 1.0f);
            drawIris({ door.x, door.y }, IRIS_MAX_RADIUS * (1.0f - easeInOut(close)));
        }
    }
    else {
        // ---- hand-off: the playfield exactly as the game draws it (no camera), iris opening on the Lair ----
        Renderer::setView(glm::mat4(1.0f));
        Environment::drawBackground(t);
        Environment::drawProps(t);
        lanes.draw(t);
        LairModel::draw(t);
        float open = segment(t, IRIS_OPEN_START, CUTSCENE_DURATION);
        drawIris(lairDoor, IRIS_MAX_RADIUS * easeInOut(open));
    }

    // letterbox bars, slide away while the iris opens so the last frame is the game framing
    float bar = LETTERBOX * (1.0f - easeInOut(segment(t, IRIS_OPEN_START, CUTSCENE_DURATION)));
    if (bar > 0.5f) {
        Renderer::drawRect({ WORLD_WIDTH * 0.5f, bar * 0.5f }, { WORLD_WIDTH, bar }, BLACK);
        Renderer::drawRect({ WORLD_WIDTH * 0.5f, WORLD_HEIGHT - bar * 0.5f }, { WORLD_WIDTH, bar }, BLACK);
    }

    // setting caption in the top bar during the establishing shot
    float caption = fadeWindow(t, 0.4f, 2.9f, 0.4f);
    if (caption > 0.0f)
        Text::drawCentered("NEW YORK CITY  -  THE SEWERS, BENEATH THE STREETS", WORLD_WIDTH * 0.5f, WORLD_HEIGHT - 36.0f, 0.5f,
                           withAlpha(hexColor(0xD8E0D0), caption));

    // subtitle: coloured speaker name + white line, centred in the bottom bar
    for (const Line& l : lines) {
        float a = fadeWindow(t, l.start, l.end, 0.15f);
        if (a <= 0.0f) continue;
        std::string name = std::string(speakerName(l.who)) + ":  ";
        float total = Text::width(name, SUB_SCALE) + Text::width(l.text, SUB_SCALE);
        float x = WORLD_WIDTH * 0.5f - total * 0.5f;
        Text::draw(name, x, 14.0f, SUB_SCALE, withAlpha(speakerColor(l.who), a));
        Text::draw(l.text, x + Text::width(name, SUB_SCALE), 14.0f, SUB_SCALE, withAlpha(WHITE, a));
    }

    // fade in from the menu
    float fadeIn = 1.0f - segment(t, 0.0f, FADE_IN_END);
    if (fadeIn > 0.0f)
        Renderer::drawRect({ WORLD_WIDTH * 0.5f, WORLD_HEIGHT * 0.5f }, { WORLD_WIDTH, WORLD_HEIGHT }, withAlpha(BLACK, fadeIn));
}
