#include "models/TurtleModels.h"
#include "core/Renderer.h"
#include "core/Mesh.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "Config.h"
#include <cmath>
#include <vector>

// Leonardo, Raphael, Donatello (req 69, 70, 72, 73, 76, 77, NF17).
// Every turtle is a tree of parts: feet centre -> pelvis -> legs (hip -> knee -> ankle)
//                                                     -> torso -> shell, plastron, belt, head, arms (shoulder -> elbow -> hand -> weapon)
// Each child matrix = parent matrix * its own translate/rotate, so turning a shoulder carries the
// forearm, hand and weapon with it (learnopengl Getting-started/Transformations, Guest-Articles/2021/
// Scene/Scene-Graph, Lab 4 orbiting cube). Circles are Lab 3 triangle fans, custom outlines are
// createPolygonMesh fans built once in init().
// Angles: a limb hangs along -y, a positive angle swings it forward (+x, the way turtles face).

// ---------- colours ----------
static glm::vec4 shade(const glm::vec4& c, float f) { return glm::vec4(glm::vec3(c) * f, c.a); }

const glm::vec4 SKIN_BACK     = shade(TURTLE_SKIN, 0.72f);   // far arm / far leg, gives depth
const glm::vec4 SKIN_LINE     = shade(TURTLE_SKIN, 0.55f);   // mouth, nostril
const glm::vec4 SHELL_RIM     = shade(SHELL, 0.55f);
const glm::vec4 SHELL_PLATE   = shade(SHELL, 1.3f);
const glm::vec4 PLASTRON_LINE = shade(PLASTRON, 0.72f);
const glm::vec4 BELT_LEATHER  = hexColor(0x5A3A1E);
const glm::vec4 PAD_LEATHER   = hexColor(0x6E4B2A);
const glm::vec4 BRASS         = hexColor(0xC9A24A);
const glm::vec4 STEEL         = hexColor(0xCDD5DD);
const glm::vec4 STEEL_DARK    = hexColor(0x7E8892);
const glm::vec4 KATANA_WRAP   = hexColor(0x1B2440);
const glm::vec4 CLOTH_WRAP    = hexColor(0xE6DAC0);
const glm::vec4 SAI_GRIP      = hexColor(0x5A2E1A);
const glm::vec4 STAFF_WOOD    = hexColor(0x8B5A2B);
const glm::vec4 STAFF_CAP     = hexColor(0x3B2A1C);
const glm::vec4 IRON          = hexColor(0x5A6064);
const glm::vec4 IRON_DARK     = hexColor(0x3A3F43);
const glm::vec4 IRON_LIGHT    = hexColor(0x8C949A);
const glm::vec4 PIPE_COPPER   = hexColor(0x9A5E36);
const glm::vec4 GOGGLE_FRAME  = hexColor(0x34383C);
const glm::vec4 GOGGLE_LENS   = hexColor(0x9ED8E6);

const float PI = 3.14159265f;
const float SOLE_HEIGHT = 5.0f;          // sole -> ankle
const float RAPHAEL_RELEASE = STRIKE_POINT;   // throw progress where the sai leaves his hand (= when Turtle.cpp spawns it)
const float RAPHAEL_REDRAW = 0.80f;      // a new sai appears in his hand from here on
const float CROUCH_SQUASH = 0.9f;        // crouch pose + a little squash to fit DUCK_HEIGHT_SCALE

// Body proportions: this is what makes the three silhouettes differ (slim / bulky + hunched / wide).
struct Build
{
    float thigh, shin, legWidth;
    float shellW, shellH;
    float plastronW, plastronH;
    float upperArm, foreArm, armWidth;
    glm::vec2 frontShoulder, backShoulder, neck;   // on the torso, origin = hips
    float headScale;
    glm::vec4 mask;
};

const Build LEO_BUILD  = { 19.0f, 18.0f, 11.0f, 52.0f, 60.0f, 32.0f, 52.0f, 19.0f, 17.0f, 9.5f,
                           { 9.0f, 47.0f }, { -3.0f, 49.0f }, { 9.0f, 56.0f }, 1.0f, LEONARDO_MASK };
const Build RAPH_BUILD = { 16.0f, 15.0f, 14.5f, 66.0f, 66.0f, 40.0f, 56.0f, 19.0f, 17.0f, 13.0f,
                           { 11.0f, 50.0f }, { -1.0f, 52.0f }, { 14.0f, 59.0f }, 1.06f, RAPHAEL_MASK };
const Build DON_BUILD  = { 19.0f, 19.0f, 13.5f, 58.0f, 64.0f, 36.0f, 56.0f, 19.0f, 17.0f, 11.0f,
                           { 9.0f, 49.0f }, { -2.0f, 51.0f }, { 10.0f, 58.0f }, 1.0f, DONATELLO_MASK };

enum class Face { Calm, Scowl, Goggles };

struct LegPose { float thigh, knee; };

// custom outlines, made once in init()
static Mesh hexPlateMesh;      // shell scute
static Mesh katanaBladeMesh;   // straight blade with an angled tip
static Mesh saiBladeMesh;      // long four-sided spike
static Mesh maskTailMesh;      // tapered ribbon

// ---------- small helpers ----------

// Piecewise keyframes with smoothstep easing between them (the attack timelines).
static float keyframes(float t, const float* times, const float* values, int count)
{
    if (t <= times[0]) return values[0];
    for (int i = 1; i < count; ++i) {
        if (t <= times[i]) {
            float s = (t - times[i - 1]) / (times[i] - times[i - 1]);
            s = s * s * (3.0f - 2.0f * s);
            return values[i - 1] + (values[i] - values[i - 1]) * s;
        }
    }
    return values[count - 1];
}

static float legHeight(const Build& b, LegPose leg)
{
    return b.thigh * cosf(glm::radians(leg.thigh)) + b.shin * cosf(glm::radians(leg.thigh + leg.knee)) + SOLE_HEIGHT;
}

// Leg poses for the walking turtles: idle stance, walk cycle, jump tuck, duck crouch.
static void walkerLegs(const Pose& pose, float stance, LegPose& front, LegPose& back)
{
    if (pose.inAir) {
        front = { 70.0f, -100.0f };
        back = { 15.0f, -85.0f };
    }
    else if (pose.ducking) {
        front = { 82.0f, -138.0f };
        back = { 28.0f, -128.0f };
    }
    else if (pose.walkCycle != 0.0f) {
        float w = pose.walkCycle;
        front = { 26.0f * sinf(w), -8.0f - 30.0f * fmaxf(0.0f, cosf(w)) };
        back = { -26.0f * sinf(w), -8.0f - 30.0f * fmaxf(0.0f, -cosf(w)) };
    }
    else {
        front = { stance, -stance * 1.2f };
        back = { -stance * 0.7f, -4.0f };
    }
}

// Hip height so the lower foot touches the floor (in the air: keep the standing height, legs tuck up).
static float hipHeight(const Build& b, const Pose& pose, LegPose front, LegPose back)
{
    if (pose.inAir) return b.thigh + b.shin + SOLE_HEIGHT - 4.0f;
    return fmaxf(legHeight(b, front), legHeight(b, back));
}

// ---------- body parts ----------

// A limb segment with a thin dark outline (bigger dark copy underneath) so arms read against the body.
static void limb(const glm::mat4& m, Shape shape, float x, float y, float w, float h, const glm::vec4& skin)
{
    Renderer::draw(shape, part(m, x, y, w + 2.4f, h + 2.4f), SKIN_LINE);
    Renderer::draw(shape, part(m, x, y, w, h), skin);
}

static void drawLeg(const glm::mat4& pelvis, float hipX, LegPose leg, const Build& b, const glm::vec4& skin)
{
    float w = b.legWidth;
    glm::mat4 hip = joint(pelvis, hipX, 0.0f, leg.thigh);
    limb(hip, Shape::Quad, 0.0f, -b.thigh * 0.5f, w, b.thigh + 2.0f, skin);

    glm::mat4 knee = joint(hip, 0.0f, -b.thigh, leg.knee);
    limb(knee, Shape::Quad, 0.0f, -b.shin * 0.5f, w * 0.88f, b.shin, skin);
    Renderer::draw(Shape::Circle, part(knee, 0.0f, 0.0f, w, w), skin);

    // ankle cancels the leg rotation so the foot stays flat on the floor
    glm::mat4 ankle = joint(knee, 0.0f, -b.shin, -(leg.thigh + leg.knee));
    limb(ankle, Shape::Circle, 4.0f, -2.0f, w * 1.75f, SOLE_HEIGHT * 1.7f, skin);
    Renderer::draw(Shape::Quad, part(ankle, 4.0f + w * 0.65f, -3.2f, 1.4f, 3.0f), SKIN_LINE);   // toe split

    // knee pad on the front of the knee (strap + pad)
    Renderer::draw(Shape::Quad, part(knee, 0.0f, 0.0f, w * 1.05f, 2.5f), BELT_LEATHER);
    Renderer::draw(Shape::Circle, part(knee, w * 0.32f, 0.0f, w * 0.8f, w * 0.95f), PAD_LEATHER);
}

// Draws shoulder -> upper arm -> elbow -> forearm and returns the hand matrix (weapons hang off it).
// The fist itself is drawn by drawHand afterwards so it sits on top of the weapon grip.
static glm::mat4 drawArm(const glm::mat4& torso, glm::vec2 shoulder, float shoulderDeg, float elbowDeg,
                         const Build& b, const glm::vec4& skin, bool wristWraps)
{
    float w = b.armWidth;
    glm::mat4 sh = joint(torso, shoulder.x, shoulder.y, shoulderDeg);
    limb(sh, Shape::Circle, 0.0f, 0.0f, w * 1.2f, w * 1.2f, skin);
    limb(sh, Shape::Quad, 0.0f, -b.upperArm * 0.5f, w, b.upperArm, skin);

    glm::mat4 elbow = joint(sh, 0.0f, -b.upperArm, elbowDeg);
    limb(elbow, Shape::Quad, 0.0f, -b.foreArm * 0.5f, w * 0.92f, b.foreArm, skin);
    Renderer::draw(Shape::Circle, part(elbow, 0.0f, 0.0f, w, w), skin);
    Renderer::draw(Shape::Circle, part(elbow, -w * 0.2f, 0.0f, w * 0.85f, w * 0.75f), PAD_LEATHER);   // elbow pad

    if (wristWraps) {
        Renderer::draw(Shape::Quad, part(elbow, 0.0f, -b.foreArm + 3.0f, w * 1.04f, 3.0f), CLOTH_WRAP);
        Renderer::draw(Shape::Quad, part(elbow, 0.0f, -b.foreArm + 7.5f, w * 1.04f, 3.0f), CLOTH_WRAP);
    }
    return translate2D(elbow, 0.0f, -b.foreArm);
}

static void drawHand(const glm::mat4& hand, const Build& b, const glm::vec4& skin)
{
    float s = b.armWidth * 1.25f;
    limb(hand, Shape::Circle, 0.0f, 0.0f, s, s, skin);
    Renderer::draw(Shape::Quad, part(hand, s * 0.15f, -s * 0.18f, s * 0.55f, 1.2f), SKIN_LINE);   // finger line
}

// Shell seen from the side: dark rim, shell, three hexagon scutes along the back.
static void drawShell(const glm::mat4& torso, const Build& b)
{
    float cx = -b.shellW * 0.2f;
    float cy = b.shellH * 0.5f - 3.0f;
    Renderer::draw(Shape::Circle, part(torso, cx, cy, b.shellW, b.shellH), SHELL_RIM);
    Renderer::draw(Shape::Circle, part(torso, cx + 1.5f, cy, b.shellW - 7.0f, b.shellH - 7.0f), SHELL);

    float plate = b.shellW * 0.3f;
    for (int i = -1; i <= 1; ++i) {
        float px = cx - b.shellW * 0.16f - (i == 0 ? 2.0f : 0.0f);
        float py = cy + i * b.shellH * 0.27f;
        Renderer::draw(hexPlateMesh, part(rotate2D(translate2D(torso, px, py), 90.0f), 0.0f, 0.0f, plate, plate), SHELL_RIM);
        Renderer::draw(hexPlateMesh, part(rotate2D(translate2D(torso, px, py), 90.0f), 0.0f, 0.0f, plate * 0.72f, plate * 0.72f), SHELL_PLATE);
    }
    // marginal notches on the back edge
    for (int i = -2; i <= 2; ++i)
        Renderer::draw(Shape::Quad, part(torso, cx - b.shellW * 0.47f, cy + i * b.shellH * 0.16f, 5.0f, 1.6f, i * 18.0f), SHELL_RIM);
}

// Side of the body, plastron with its seams, belt with buckle and a hanging end.
static void drawFront(const glm::mat4& torso, const Build& b)
{
    float cy = b.shellH * 0.5f - 3.0f;
    float px = b.plastronW * 0.2f;
    Renderer::draw(Shape::Circle, part(torso, px - 6.0f, cy + 2.0f, b.plastronW + 8.0f, b.shellH - 2.0f), TURTLE_SKIN);
    Renderer::draw(Shape::Circle, part(torso, px, cy, b.plastronW, b.plastronH), PLASTRON);
    for (int i = -1; i <= 2; ++i) {
        float y = cy + i * b.plastronH * 0.2f;
        float half = sqrtf(fmaxf(0.0f, 1.0f - powf((y - cy) / (b.plastronH * 0.5f), 2.0f)));   // chord of the ellipse
        Renderer::draw(Shape::Quad, part(torso, px, y, b.plastronW * half * 0.82f, 1.6f), PLASTRON_LINE);
    }
    Renderer::draw(Shape::Quad, part(torso, px + b.plastronW * 0.12f, cy, 1.4f, b.plastronH * 0.75f), PLASTRON_LINE);

    float beltLeft = -b.shellW * 0.62f;
    float beltRight = px + b.plastronW * 0.5f;
    Renderer::draw(Shape::Quad, part(torso, (beltLeft + beltRight) * 0.5f, 5.0f, beltRight - beltLeft, 7.0f), BELT_LEATHER);
    Renderer::draw(Shape::Quad, part(torso, beltLeft + 6.0f, -1.0f, 4.0f, 11.0f, -15.0f), BELT_LEATHER);   // knot end
    Renderer::draw(Shape::Quad, part(torso, beltRight - 6.0f, 5.0f, 8.0f, 9.0f), BRASS);
    Renderer::draw(Shape::Quad, part(torso, beltRight - 6.0f, 5.0f, 4.0f, 5.0f), BELT_LEATHER);
}

// Head on a neck joint: skull, snout, mask band + knot + two fluttering tails, eye, mouth,
// and the per-turtle extra (Raphael's scowl brow, Donatello's goggles).
static void drawHead(const glm::mat4& torso, const Build& b, float tilt, Face face, float time, float flutter)
{
    Renderer::draw(Shape::Quad, part(torso, b.neck.x - 1.0f, b.neck.y - 3.0f, 12.0f, 12.0f, -10.0f), TURTLE_SKIN);
    glm::mat4 head = scale2D(joint(torso, b.neck.x, b.neck.y, tilt), b.headScale, b.headScale);

    // mask tails first so they hang behind the head
    float wave = sinf(time * 5.0f) * 6.0f * flutter;
    glm::mat4 knot = translate2D(head, -12.5f, 13.0f);
    Renderer::draw(maskTailMesh, rotate2D(knot, 200.0f + wave), shade(b.mask, 0.8f));
    Renderer::draw(maskTailMesh, scale2D(rotate2D(knot, 222.0f + wave * 1.4f), 0.8f, 0.9f), shade(b.mask, 0.68f));

    Renderer::draw(Shape::Circle, part(head, 0.0f, 11.0f, 28.0f, 26.0f), TURTLE_SKIN);
    Renderer::draw(Shape::Circle, part(head, 9.5f, 5.0f, 19.0f, 13.5f), TURTLE_SKIN);
    Renderer::draw(Shape::Quad, part(head, 1.5f, 12.5f, 29.5f, 9.0f, -4.0f), b.mask);
    Renderer::draw(Shape::Circle, part(head, -12.5f, 13.0f, 7.0f, 7.0f), shade(b.mask, 0.85f));

    // eye with a blink every few seconds
    float blinkPhase = fmodf(time + b.headScale * 7.0f, 3.7f);
    float eyeH = blinkPhase < 0.12f ? 1.2f : 6.5f;
    Renderer::draw(Shape::Circle, part(head, 9.0f, 12.5f, 6.5f, eyeH), WHITE);
    if (eyeH > 2.0f) Renderer::draw(Shape::Circle, part(head, 10.6f, 12.3f, 2.6f, 3.4f), BLACK);

    Renderer::draw(Shape::Quad, part(head, 12.5f, 1.5f, 8.0f, 1.4f, face == Face::Scowl ? -10.0f : 10.0f), SKIN_LINE);
    Renderer::draw(Shape::Circle, part(head, 17.5f, 7.0f, 1.6f, 1.6f), SKIN_LINE);

    if (face == Face::Scowl) {
        // heavy brow slanting down toward the snout
        Renderer::draw(Shape::Quad, part(head, 8.5f, 17.0f, 11.0f, 3.6f, -22.0f), shade(b.mask, 0.5f));
    }
    else if (face == Face::Goggles) {
        // goggles pushed up on the forehead: strap, frame, lens, glint
        Renderer::draw(Shape::Quad, part(head, -1.0f, 20.0f, 27.0f, 3.2f, -6.0f), GOGGLE_FRAME);
        Renderer::draw(Shape::Circle, part(head, 6.5f, 21.0f, 10.0f, 9.0f), GOGGLE_FRAME);
        Renderer::draw(Shape::Circle, part(head, 7.0f, 21.0f, 6.8f, 6.0f), GOGGLE_LENS);
        Renderer::draw(Shape::Circle, part(head, 8.5f, 22.5f, 1.8f, 1.8f), WHITE);
    }
}

// ---------- weapons (children of a hand matrix, pointing along +x) ----------

static void drawKatana(const glm::mat4& hand, float degrees)
{
    glm::mat4 k = rotate2D(hand, degrees);
    Renderer::draw(Shape::Quad, part(k, -1.5f, 0.0f, 17.0f, 4.4f), KATANA_WRAP);              // hilt
    for (int i = 0; i < 3; ++i)
        Renderer::draw(Shape::Quad, part(k, -7.5f + i * 4.5f, 0.0f, 2.2f, 2.2f, 45.0f), CLOTH_WRAP);   // diamond wraps
    Renderer::draw(Shape::Circle, part(k, -10.5f, 0.0f, 4.5f, 5.0f), BRASS);                  // pommel
    Renderer::draw(Shape::Quad, part(k, 8.5f, 0.0f, 3.0f, 10.0f), BRASS);                      // guard
    Renderer::draw(katanaBladeMesh, translate2D(k, 10.0f, 0.0f), STEEL);                        // blade
    Renderer::draw(Shape::Quad, part(k, 33.0f, 1.3f, 44.0f, 1.0f), STEEL_DARK);                 // spine line
}

static void drawSaiInHand(const glm::mat4& hand, float degrees, float size)
{
    glm::mat4 s = scale2D(rotate2D(hand, degrees), size, size);
    Renderer::draw(Shape::Quad, part(s, -3.5f, 0.0f, 13.0f, 4.2f), SAI_GRIP);
    Renderer::draw(Shape::Quad, part(s, -6.0f, 0.0f, 1.6f, 4.6f), CLOTH_WRAP);
    Renderer::draw(Shape::Quad, part(s, -1.5f, 0.0f, 1.6f, 4.6f), CLOTH_WRAP);
    Renderer::draw(Shape::Circle, part(s, -10.5f, 0.0f, 4.5f, 4.5f), STEEL_DARK);                 // pommel
    Renderer::draw(saiBladeMesh, translate2D(s, 3.0f, 0.0f), STEEL);                                // centre blade
    for (int side = -1; side <= 1; side += 2) {
        Renderer::draw(Shape::Quad, part(s, 3.5f, side * 3.5f, 3.0f, 6.0f), STEEL);                // prong stub
        Renderer::draw(Shape::Triangle, part(s, 8.0f, side * 6.3f, 2.8f, 10.0f, -90.0f + side * 12.0f), STEEL);   // prong
    }
}

static void drawBoStaff(const glm::mat4& hand, float degrees, float below, float above)
{
    glm::mat4 s = rotate2D(hand, degrees);
    float len = below + above;
    float mid = (above - below) * 0.5f;
    Renderer::draw(Shape::Quad, part(s, mid, 0.0f, len, 5.5f), STAFF_WOOD);
    Renderer::draw(Shape::Quad, part(s, mid, 1.2f, len, 1.2f), shade(STAFF_WOOD, 1.2f));         // grain highlight
    Renderer::draw(Shape::Quad, part(s, -below + 3.0f, 0.0f, 6.0f, 6.5f), STAFF_CAP);            // end caps
    Renderer::draw(Shape::Quad, part(s, above - 3.0f, 0.0f, 6.0f, 6.5f), STAFF_CAP);
    for (int i = 0; i < 3; ++i) {                                                                 // grip wraps
        Renderer::draw(Shape::Quad, part(s, above - 18.0f - i * 4.0f, 0.0f, 2.4f, 6.2f), CLOTH_WRAP);
        Renderer::draw(Shape::Quad, part(s, -below + 18.0f + i * 4.0f, 0.0f, 2.4f, 6.2f), CLOTH_WRAP);
    }
}

// Manhole cover seen at an angle (squashed in x), with pipe straps bolted across it.
static void drawShield(const glm::mat4& center)
{
    glm::mat4 m = scale2D(center, 0.72f, 1.0f);
    const float D = 58.0f;
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, D, D), IRON_DARK);                 // rim
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, D - 7.0f, D - 7.0f), IRON);
    Renderer::draw(Shape::Ring, part(m, 0.0f, 0.0f, D * 0.62f, D * 0.62f), IRON_DARK);     // inner ring
    for (int i = -2; i <= 2; ++i)                                                          // grip slots
        Renderer::draw(Shape::Quad, part(m, 0.0f, i * 6.0f, D * 0.42f - fabsf((float)i) * 4.0f, 2.2f), IRON_DARK);
    for (int i = 0; i < 8; ++i) {                                                          // rim bolts
        float a = i * PI * 0.25f + PI / 8.0f;
        Renderer::draw(Shape::Circle, part(m, cosf(a) * D * 0.43f, sinf(a) * D * 0.43f, 3.6f, 3.6f), IRON_LIGHT);
    }
    // pipe straps (bent pipe welded across the cover) with flanges
    Renderer::draw(Shape::Quad, part(m, 0.0f, D * 0.25f, D * 0.86f, 4.5f, -8.0f), PIPE_COPPER);
    Renderer::draw(Shape::Quad, part(m, 0.0f, -D * 0.25f, D * 0.86f, 4.5f, 8.0f), PIPE_COPPER);
    for (int side = -1; side <= 1; side += 2) {
        Renderer::draw(Shape::Quad, part(m, side * D * 0.4f, D * 0.25f - side * 2.5f, 3.0f, 7.5f, -8.0f), shade(PIPE_COPPER, 0.7f));
        Renderer::draw(Shape::Quad, part(m, side * D * 0.4f, -D * 0.25f + side * 2.5f, 3.0f, 7.5f, 8.0f), shade(PIPE_COPPER, 0.7f));
    }
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 9.0f, 9.0f), IRON_LIGHT);           // centre boss
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 5.0f, 5.0f), IRON_DARK);
}

// ---------- init / shutdown ----------

void TurtleModels::init()
{
    std::vector<glm::vec2> hex;
    for (int i = 0; i < 6; ++i) hex.push_back(glm::vec2(cosf(i * PI / 3.0f), sinf(i * PI / 3.0f)) * 0.5f);
    hexPlateMesh = createPolygonMesh(hex);

    katanaBladeMesh = createPolygonMesh({ { 0.0f, -2.0f }, { 47.0f, -2.0f }, { 55.0f, 0.4f }, { 47.0f, 2.3f }, { 0.0f, 2.0f } });
    saiBladeMesh = createPolygonMesh({ { 0.0f, -1.8f }, { 22.0f, -1.1f }, { 28.0f, 0.0f }, { 22.0f, 1.1f }, { 0.0f, 1.8f } });
    maskTailMesh = createPolygonMesh({ { 0.0f, 3.0f }, { 15.0f, 2.6f }, { 25.0f, 0.4f }, { 27.0f, -1.2f }, { 15.0f, -2.6f }, { 0.0f, -3.0f } });
}

void TurtleModels::shutdown()
{
    destroyMesh(hexPlateMesh);
    destroyMesh(katanaBladeMesh);
    destroyMesh(saiBladeMesh);
    destroyMesh(maskTailMesh);
}

// ---------- Leonardo: slim, upright, two katanas ----------
// Attack = overhead slash: front arm winds up over the head, then the katana sweeps down in front
// around the shoulder joint (req 70: the weapon animates in his hand), level in front at STRIKE_POINT.

void TurtleModels::drawLeonardo(const glm::mat4& base, const Pose& pose)
{
    const Build& b = LEO_BUILD;
    LegPose front, back;
    walkerLegs(pose, 10.0f, front, back);
    float hipY = hipHeight(b, pose, front, back);

    static const float T[]        = { 0.0f, 0.22f, STRIKE_POINT, 0.64f, 1.0f };   // blade level in front on the strike frame
    static const float SHOULDER[] = { 32.0f, 158.0f, 66.0f, 52.0f, 32.0f };
    static const float ELBOW[]    = { 52.0f, 36.0f, 8.0f, 22.0f, 52.0f };
    static const float BLADE[]    = { -24.0f, 18.0f, -82.0f, -62.0f, -24.0f };
    static const float LEAN[]     = { -3.0f, 3.0f, -10.0f, -8.0f, -3.0f };
    static const float BACKARM[]  = { -22.0f, -10.0f, -48.0f, -40.0f, -22.0f };
    float a = pose.attack;
    float shoulder = keyframes(a, T, SHOULDER, 5);
    float elbow = keyframes(a, T, ELBOW, 5);
    float blade = keyframes(a, T, BLADE, 5);
    float lean = keyframes(a, T, LEAN, 5);
    float backShoulder = keyframes(a, T, BACKARM, 5);
    float backElbow = 26.0f;
    float pelvisX = 0.0f;
    float headTilt = 2.0f;

    if (a <= 0.0f) {
        if (pose.ducking) { shoulder = 72.0f; elbow = 18.0f; blade = -84.0f; backShoulder = -45.0f; backElbow = 40.0f; }
        else if (pose.inAir) { shoulder = 118.0f; elbow = 30.0f; blade = -70.0f; backShoulder = -60.0f; backElbow = 20.0f; }
        else {
            float swing = pose.walkCycle != 0.0f ? sinf(pose.walkCycle) : 0.0f;
            shoulder += sinf(pose.time * 2.4f) * 3.0f - swing * 8.0f;
            backShoulder += swing * 10.0f;
        }
    }
    if (pose.ducking) { lean -= 38.0f; pelvisX = -10.0f; headTilt = 30.0f; }
    if (pose.inAir) lean += 6.0f;
    lean += pose.hitFlash * 8.0f;
    if (pose.ducking && a <= 0.0f) blade = 4.0f - (lean + shoulder + elbow);   // keep the katana level, not in the floor

    glm::mat4 body = pose.ducking ? scale2D(base, 1.0f, CROUCH_SQUASH) : base;
    glm::mat4 pelvis = translate2D(body, pelvisX, hipY);
    float bob = pose.ducking || pose.inAir ? 0.0f : sinf(pose.time * 3.0f) * 1.2f;
    glm::mat4 torso = rotate2D(translate2D(pelvis, 0.0f, bob), lean);

    // far arm: second katana held low and trailing behind
    glm::mat4 backHand = drawArm(torso, b.backShoulder, backShoulder, backElbow, b, SKIN_BACK, false);
    drawKatana(backHand, -138.0f - backShoulder * 0.2f - backElbow);
    drawHand(backHand, b, SKIN_BACK);

    drawLeg(pelvis, -4.0f, back, b, SKIN_BACK);
    drawShell(torso, b);
    drawLeg(pelvis, 4.0f, front, b, TURTLE_SKIN);
    drawFront(torso, b);
    drawHead(torso, b, headTilt - lean * 0.3f, Face::Calm, pose.time, pose.walkCycle != 0.0f || pose.inAir ? 1.6f : 1.0f);

    // near arm: the swinging katana
    glm::mat4 hand = drawArm(torso, b.frontShoulder, shoulder, elbow, b, TURTLE_SKIN, false);
    drawKatana(hand, blade);
    drawHand(hand, b, TURTLE_SKIN);
}

// ---------- Raphael: bulky, hunched, sai ----------
// Attack = throw: arm winds back over the shoulder, whips forward, the sai leaves the hand at
// RAPHAEL_RELEASE (hand empty, follow-through), then a new sai is pulled into the hand at RAPHAEL_REDRAW.

void TurtleModels::drawRaphael(const glm::mat4& base, const Pose& pose)
{
    const Build& b = RAPH_BUILD;
    LegPose front, back;
    walkerLegs(pose, 15.0f, front, back);
    float hipY = hipHeight(b, pose, front, back);

    // the arm swings down and back to -122 deg; the same angle written as 238 makes the release whip
    // forward over the top of the head (overhand) instead of back under the body
    static const float T[]        = { 0.0f, 0.33f, 0.331f, RAPHAEL_RELEASE, 0.62f, RAPHAEL_REDRAW, 1.0f };
    static const float SHOULDER[] = { 28.0f, -122.0f, 238.0f, 72.0f, 28.0f, 22.0f, 28.0f };
    static const float ELBOW[]    = { 62.0f, -40.0f, -40.0f, 4.0f, 22.0f, 58.0f, 62.0f };
    static const float SAI[]      = { -48.0f, -58.0f, -58.0f, -86.0f, -60.0f, -48.0f, -48.0f };
    static const float LEAN[]     = { -14.0f, -4.0f, -4.0f, -24.0f, -22.0f, -15.0f, -14.0f };
    static const float BACKARM[]  = { 72.0f, 92.0f, 92.0f, 30.0f, 40.0f, 68.0f, 72.0f };
    float a = pose.attack;
    float shoulder = keyframes(a, T, SHOULDER, 7);
    float elbow = keyframes(a, T, ELBOW, 7);
    float sai = keyframes(a, T, SAI, 7);
    float lean = keyframes(a, T, LEAN, 7);
    float backShoulder = keyframes(a, T, BACKARM, 7);
    float backElbow = 58.0f;
    float pelvisX = 0.0f;
    float headTilt = 6.0f;

    if (a <= 0.0f) {
        if (pose.ducking) { shoulder = 60.0f; elbow = 50.0f; backShoulder = 70.0f; backElbow = 70.0f; }
        else if (pose.inAir) { shoulder = 130.0f; elbow = 40.0f; backShoulder = 100.0f; backElbow = 50.0f; }
        else {
            float swing = pose.walkCycle != 0.0f ? sinf(pose.walkCycle) : 0.0f;
            shoulder += sinf(pose.time * 2.0f) * 4.0f - swing * 10.0f;
            backShoulder += swing * 12.0f + sinf(pose.time * 2.0f + 1.0f) * 3.0f;
        }
    }
    if (pose.ducking) { lean -= 30.0f; pelvisX = -8.0f; headTilt = 26.0f; }
    if (pose.inAir) lean += 8.0f;
    lean += pose.hitFlash * 8.0f;
    if (pose.ducking && a <= 0.0f) sai = 10.0f - (lean + shoulder + elbow);

    glm::mat4 body = pose.ducking ? scale2D(base, 1.0f, CROUCH_SQUASH) : base;
    glm::mat4 pelvis = translate2D(body, pelvisX, hipY);
    float bob = pose.ducking || pose.inAir ? 0.0f : sinf(pose.time * 2.6f) * 1.5f;
    glm::mat4 torso = rotate2D(translate2D(pelvis, 0.0f, bob), lean);

    // far arm: clenched fist up in a brawler's guard
    glm::mat4 backHand = drawArm(torso, b.backShoulder, backShoulder, backElbow, b, SKIN_BACK, true);
    drawHand(backHand, b, SKIN_BACK);

    drawLeg(pelvis, -5.0f, back, b, SKIN_BACK);
    drawShell(torso, b);
    drawLeg(pelvis, 5.0f, front, b, TURTLE_SKIN);
    drawFront(torso, b);

    // holster on the belt: leather sheath with a spare sai, handle up
    glm::mat4 holster = joint(torso, -b.shellW * 0.42f, 2.0f, -112.0f);
    drawSaiInHand(holster, 0.0f, 0.8f);
    Renderer::draw(Shape::Quad, part(holster, 9.0f, 0.0f, 16.0f, 7.5f), BELT_LEATHER);
    Renderer::draw(Shape::Quad, part(holster, 2.0f, 0.0f, 2.0f, 8.0f), BRASS);

    drawHead(torso, b, headTilt - lean * 0.75f, Face::Scowl, pose.time, pose.walkCycle != 0.0f || pose.inAir ? 1.6f : 1.0f);

    // near arm: the throwing sai (gone between release and redraw, then grows back into the fist)
    glm::mat4 hand = drawArm(torso, b.frontShoulder, shoulder, elbow, b, TURTLE_SKIN, true);
    bool holding = a <= 0.0f || a < RAPHAEL_RELEASE || a >= RAPHAEL_REDRAW;
    if (holding) {
        float size = a >= RAPHAEL_REDRAW ? fminf(1.0f, 0.4f + (a - RAPHAEL_REDRAW) * 6.0f) : 1.0f;
        drawSaiInHand(hand, sai, size * 1.15f);
    }
    drawHand(hand, b, TURTLE_SKIN);
}

// ---------- Donatello: wide planted guard, bo staff + manhole shield ----------
// Never walks, jumps or ducks (req 78), so those pose flags are ignored. Idle = slow guard sway,
// hitFlash = block recoil (shield knocked toward him, body rocks back, staff jolts).
// attack (unused in play) = shield slam, for the cutscene standoff.

void TurtleModels::drawDonatello(const glm::mat4& base, const Pose& pose)
{
    const Build& b = DON_BUILD;
    LegPose front = { 24.0f, -30.0f };
    LegPose back = { -26.0f, -6.0f };
    float sway = sinf(pose.time * 1.6f);
    float recoil = pose.hitFlash;
    float slam = pose.attack > 0.0f ? sinf(pose.attack * PI) : 0.0f;
    front.knee -= sway * 3.0f;
    float hipY = fmaxf(legHeight(b, front), legHeight(b, back));

    glm::mat4 pelvis = translate2D(base, 2.0f, hipY);
    float lean = sway * 2.0f + recoil * 9.0f - slam * 6.0f;
    glm::mat4 torso = rotate2D(translate2D(pelvis, -recoil * 3.0f, sway * 0.8f), lean);

    drawLeg(pelvis, -6.0f, back, b, SKIN_BACK);
    drawShell(torso, b);
    drawLeg(pelvis, 6.0f, front, b, TURTLE_SKIN);
    drawFront(torso, b);

    // far arm reaches across the chest and braces the staff diagonally (low in front, high behind)
    float backShoulder = 38.0f + sway * 2.0f;
    float backElbow = 66.0f;
    glm::mat4 backHand = drawArm(torso, b.backShoulder, backShoulder, backElbow, b, SKIN_BACK, false);
    float staffAngle = 110.0f + recoil * 14.0f - sway * 2.0f;
    drawBoStaff(backHand, staffAngle - (lean + backShoulder + backElbow), 80.0f, 74.0f);
    drawHand(backHand, b, SKIN_BACK);

    drawHead(torso, b, 2.0f - lean * 0.5f, Face::Goggles, pose.time, 0.8f);

    // near arm holds the shield out front; on a hit it's knocked back toward the body
    float shoulder = 40.0f - recoil * 22.0f + slam * 40.0f + sway * 2.0f;
    float elbow = 52.0f - recoil * 10.0f;
    glm::mat4 hand = drawArm(torso, b.frontShoulder, shoulder, elbow, b, TURTLE_SKIN, false);
    drawHand(hand, b, TURTLE_SKIN);
    // undo the arm rotation so the cover stays upright, lean it back a little on impact
    glm::mat4 shield = joint(hand, 0.0f, 0.0f, -(lean + shoulder + elbow) + recoil * 8.0f);
    drawShield(translate2D(shield, 3.0f, -3.0f));
}

void TurtleModels::draw(TurtleKind kind, const glm::mat4& base, const Pose& pose)
{
    switch (kind) {
    case TurtleKind::Leonardo:  drawLeonardo(base, pose);  break;
    case TurtleKind::Raphael:   drawRaphael(base, pose);   break;
    case TurtleKind::Donatello: drawDonatello(base, pose); break;
    default: break;
    }
}
