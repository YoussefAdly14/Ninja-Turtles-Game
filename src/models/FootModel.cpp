#include "models/VillainModels.h"
#include "core/Renderer.h"
#include "core/Mesh.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "Config.h"
#include <cmath>
#include <vector>

// Foot Soldier (req 85, 91, 123): lean, angular Foot Clan ninja. Easy enemy that throws shuriken
// from an OVERHEAD swing, so his attack reads as HIGH (turtles duck).
// Drawn facing +x, then mirrored with scale2D(base, -1, 1) so he faces left.
// Hierarchy = parent * child (learnopengl Transformations + Guest Articles Scene-Graph, Lab 4 orbiting cube):
//   hips -> torso -> shoulder -> elbow -> hand -> shuriken,   hips -> hip joint -> knee -> ankle -> foot.

const float PI = 3.14159265f;

// colours: Foot Clan dark purple/black family, nothing green like the turtles
const glm::vec4 CLOTH       = FOOT_CLAN;
const glm::vec4 CLOTH_FAR   = hexColor(0x1C1422);   // far arm/leg, so the near ones stand out
const glm::vec4 CLOTH_LIGHT = hexColor(0x45355A);   // folds, mask, gloves
const glm::vec4 ARMOUR      = hexColor(0x54456A);
const glm::vec4 ARMOUR_EDGE = hexColor(0x7C6A94);
const glm::vec4 WRAPS       = hexColor(0x5A4C6E);
const glm::vec4 SASH        = hexColor(0x8A1E2E);
const glm::vec4 EMBLEM      = hexColor(0xD02A34);
const glm::vec4 SKIN        = hexColor(0xC89F7E);
const glm::vec4 STEEL       = hexColor(0xB8BCC4);
const glm::vec4 POUCH       = hexColor(0x4A3426);

// sizes (model units = world units, feet at y = 0, about FOOT_WIDTH x FOOT_HEIGHT)
const float THIGH = 30.0f, SHIN = 26.0f, FOOT_H = 6.0f;
const float UPPER_ARM = 20.0f, FOREARM = 18.0f;
const float STRIDE = 24.0f;          // hip swing (degrees) while walking
const float SHOULDER_X = 4.0f, SHOULDER_Y = 37.0f;

// throw keys (attack progress 0..1): wind-up behind the head, whip over the top, follow through
const int   THROW_KEYS = 5;
// the release lines up with STRIKE_POINT, the frame where Villain.cpp spawns the shuriken
const float THROW_TIMES[THROW_KEYS] = { 0.0f, STRIKE_POINT * 0.6f, STRIKE_POINT, STRIKE_POINT + (1.0f - STRIKE_POINT) * 0.35f, 1.0f };

// meshes, built once in initFootSoldier()
static Mesh torsoMesh, hoodMesh, maskMesh, chestPlateMesh, pauldronMesh, starMesh;

// ---------- small helpers ----------

static float easeInOut(float t) { return t * t * (3.0f - 2.0f * t); }

// animation curve: values[i] at times[i], eased in between
static float keyframes(float t, const float* times, const float* values, int count)
{
    if (t <= times[0]) return values[0];
    for (int i = 1; i < count; ++i) {
        if (t <= times[i]) {
            float k = (t - times[i - 1]) / (times[i] - times[i - 1]);
            return values[i - 1] + (values[i] - values[i - 1]) * easeInOut(k);
        }
    }
    return values[count - 1];
}

// how far below the hip the sole is, so the lowest foot always touches the floor
static float legDrop(float hipDeg, float kneeDeg)
{
    return THIGH * cosf(glm::radians(hipDeg)) + SHIN * cosf(glm::radians(hipDeg + kneeDeg)) + FOOT_H;
}

// ---------- body parts ----------

static void drawLeg(const glm::mat4& hips, float x, float hipDeg, float kneeDeg, bool near)
{
    glm::vec4 cloth = near ? CLOTH : CLOTH_FAR;
    glm::vec4 wraps = near ? WRAPS : CLOTH_LIGHT;

    glm::mat4 hip = joint(hips, x, 0.0f, hipDeg);
    Renderer::draw(Shape::Quad, part(hip, 0.0f, -THIGH * 0.5f, 12.0f, THIGH + 4.0f), cloth);

    glm::mat4 knee = joint(hip, 0.0f, -THIGH, kneeDeg);
    Renderer::draw(Shape::Circle, part(knee, 0.0f, 0.0f, 11.0f, 11.0f), cloth);
    Renderer::draw(Shape::Quad, part(knee, 0.0f, -SHIN * 0.5f, 9.0f, SHIN), cloth);
    // shin wraps, tilted a bit like wound cloth
    Renderer::draw(Shape::Quad, part(knee, 0.0f, -SHIN * 0.4f, 10.5f, 3.0f, 10.0f), wraps);
    Renderer::draw(Shape::Quad, part(knee, 0.0f, -SHIN * 0.65f, 10.5f, 3.0f, 10.0f), wraps);

    // tabi boot stays flat on the floor: undo the hip + knee rotation
    glm::mat4 ankle = joint(knee, 0.0f, -SHIN, -(hipDeg + kneeDeg));
    Renderer::draw(Shape::Quad, part(ankle, 3.5f, -FOOT_H * 0.5f, 17.0f, FOOT_H), cloth);
    Renderer::draw(Shape::Quad, part(ankle, 8.5f, -FOOT_H * 0.5f, 1.5f, FOOT_H), CLOTH_LIGHT);   // split toe
}

static void drawShuriken(const glm::mat4& m, float size, float spin)
{
    Renderer::draw(starMesh, part(m, 0.0f, 0.0f, size, size, spin), STEEL);
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, size * 0.25f, size * 0.25f), CLOTH);
}

static void drawArm(const glm::mat4& torso, float x, float shoulderDeg, float elbowDeg, bool near, bool holdStar, float time)
{
    glm::vec4 cloth = near ? CLOTH : CLOTH_FAR;

    glm::mat4 shoulder = joint(torso, x, SHOULDER_Y, shoulderDeg);
    Renderer::draw(Shape::Quad, part(shoulder, 0.0f, -UPPER_ARM * 0.5f, 9.0f, UPPER_ARM + 4.0f), cloth);

    glm::mat4 elbow = joint(shoulder, 0.0f, -UPPER_ARM, elbowDeg);
    Renderer::draw(Shape::Circle, part(elbow, 0.0f, 0.0f, 8.5f, 8.5f), cloth);
    Renderer::draw(Shape::Quad, part(elbow, 0.0f, -FOREARM * 0.5f, 7.5f, FOREARM), cloth);
    Renderer::draw(Shape::Quad, part(elbow, 0.0f, -FOREARM * 0.62f, 9.5f, 9.0f), near ? ARMOUR_EDGE : ARMOUR);   // bracer

    glm::mat4 hand = translate2D(elbow, 0.0f, -FOREARM - 2.0f);
    Renderer::draw(Shape::Circle, part(hand, 0.0f, 0.0f, 8.5f, 8.5f), CLOTH_LIGHT);   // glove
    if (holdStar) drawShuriken(translate2D(hand, 0.0f, -4.0f), 13.0f, time * 90.0f);
}

static void drawTorso(const glm::mat4& torso, float sashSway)
{
    // sash tails fly out behind the waist
    Renderer::draw(Shape::Triangle, part(joint(torso, -10.0f, 1.0f, 145.0f + sashSway), 0.0f, 8.0f, 5.0f, 17.0f), SASH);
    Renderer::draw(Shape::Triangle, part(joint(torso, -10.0f, 0.0f, 165.0f + sashSway * 0.6f), 0.0f, 7.0f, 4.0f, 14.0f), SASH);

    Renderer::draw(torsoMesh, torso, CLOTH);

    // belt + knot
    Renderer::draw(Shape::Quad, part(torso, 0.0f, 1.0f, 25.0f, 5.5f, 4.0f), SASH);
    Renderer::draw(Shape::Circle, part(torso, 11.0f, 1.5f, 6.0f, 6.0f), SASH);

    // shuriken pouch on the back hip, one star peeking out
    drawShuriken(translate2D(torso, -10.0f, 2.5f), 8.0f, 20.0f);
    Renderer::draw(Shape::Quad, part(torso, -10.0f, -4.0f, 8.0f, 9.0f), POUCH);
    Renderer::draw(Shape::Quad, part(torso, -10.0f, -0.5f, 9.0f, 3.0f, -6.0f), hexColor(0x33231A));

    // angular armour: two belly plates + chest plate with a lighter rim
    Renderer::draw(Shape::Quad, part(torso, 4.0f, 7.0f, 15.0f, 4.5f, 6.0f), ARMOUR);
    Renderer::draw(Shape::Quad, part(torso, 5.0f, 12.5f, 16.0f, 4.5f, 6.0f), ARMOUR);
    Renderer::draw(chestPlateMesh, part(torso, 7.0f, 28.0f, 1.15f, 1.12f), ARMOUR_EDGE);
    Renderer::draw(chestPlateMesh, part(torso, 7.0f, 28.0f, 1.0f, 1.0f), ARMOUR);

    // Foot emblem (our own design): red badge with a three-claw slash
    glm::mat4 badge = translate2D(torso, 8.0f, 28.0f);
    Renderer::draw(Shape::Circle, part(badge, 0.0f, 0.0f, 11.0f, 12.0f), EMBLEM);
    for (int i = -1; i <= 1; ++i)
        Renderer::draw(Shape::Triangle, part(joint(badge, 0.0f, -4.0f, i * 26.0f), 0.0f, 4.0f, 1.5f, 8.0f), CLOTH);
}

static void drawHead(const glm::mat4& torso, float tilt, float tailFlutter)
{
    glm::mat4 neck = joint(torso, 3.0f, 41.0f, tilt);
    Renderer::draw(Shape::Quad, part(neck, 0.0f, 4.0f, 9.0f, 9.0f), CLOTH);

    glm::mat4 head = translate2D(neck, 1.0f, 14.0f);
    // hood tail hangs off the back of the head and flutters
    Renderer::draw(Shape::Triangle, part(joint(head, -11.0f, 2.0f, 125.0f + tailFlutter), 0.0f, 9.0f, 7.0f, 20.0f), CLOTH);
    Renderer::draw(hoodMesh, head, CLOTH);
    Renderer::draw(maskMesh, head, CLOTH_LIGHT);                                   // face mask over mouth and nose
    Renderer::draw(Shape::Quad, part(head, 2.5f, 2.0f, 2.5f, 15.0f, -4.0f), CLOTH_LIGHT);   // hood fold around the face

    // eye slit: strip of skin, one narrow angry eye, hood brow pulled low
    Renderer::draw(Shape::Quad, part(head, 8.5f, 3.5f, 11.0f, 5.0f, -6.0f), SKIN);
    Renderer::draw(Shape::Quad, part(head, 10.0f, 3.4f, 4.6f, 2.4f, -12.0f), WHITE);
    Renderer::draw(Shape::Quad, part(head, 11.0f, 3.2f, 1.8f, 2.4f, -12.0f), BLACK);
    Renderer::draw(Shape::Quad, part(head, 8.5f, 6.6f, 12.0f, 2.4f, -14.0f), CLOTH);
}

// ---------- init / draw ----------

void VillainModels::initFootSoldier()
{
    // V-shaped torso: broad shoulders, narrow waist (hips at 0,0)
    torsoMesh = createPolygonMesh({ { -9.0f, -3.0f }, { 9.0f, -3.0f }, { 12.0f, 14.0f }, { 15.0f, 30.0f }, { 12.0f, 40.0f },
                                    { 2.0f, 43.0f }, { -9.0f, 41.0f }, { -13.0f, 30.0f }, { -11.0f, 14.0f } });
    // hood with a sharp peak at the back (head centre at 0,0)
    hoodMesh = createPolygonMesh({ { -10.0f, -8.0f }, { -13.0f, 2.0f }, { -17.0f, 13.0f }, { -6.0f, 15.0f }, { 5.0f, 14.0f },
                                   { 12.0f, 8.0f }, { 13.0f, -2.0f }, { 10.0f, -10.0f }, { 0.0f, -12.0f } });
    maskMesh = createPolygonMesh({ { 3.0f, 0.5f }, { 13.5f, 0.5f }, { 12.5f, -6.0f }, { 9.0f, -11.0f }, { 1.0f, -10.0f } });
    chestPlateMesh = createPolygonMesh({ { -8.0f, -10.0f }, { 5.0f, -7.0f }, { 7.0f, 4.0f }, { 3.0f, 11.0f },
                                         { -7.0f, 10.0f }, { -10.0f, 0.0f } });
    pauldronMesh = createPolygonMesh({ { -7.0f, 4.0f }, { 4.0f, 6.0f }, { 10.0f, 1.0f }, { 8.0f, -8.0f },
                                       { 0.0f, -11.0f }, { -6.0f, -6.0f } });
    // 4-point throwing star, outer radius 0.5
    std::vector<glm::vec2> star;
    for (int i = 0; i < 8; ++i) {
        float a = PI * 0.25f * i;
        float r = (i % 2 == 0) ? 0.5f : 0.15f;
        star.push_back(glm::vec2(r * cosf(a), r * sinf(a)));
    }
    starMesh = createPolygonMesh(star);
}

void VillainModels::shutdownFootSoldier()
{
    destroyMesh(torsoMesh);
    destroyMesh(hoodMesh);
    destroyMesh(maskMesh);
    destroyMesh(chestPlateMesh);
    destroyMesh(pauldronMesh);
    destroyMesh(starMesh);
}

void VillainModels::drawFootSoldier(const glm::mat4& base, const Pose& pose)
{
    float hit = pose.hitFlash;
    float a = pose.attack;

    // walk: legs in a wide ninja stance at rest, arms swing against the legs
    float frontPhase = pose.walkCycle + PI * 0.5f;
    float frontHip = sinf(frontPhase) * STRIDE;
    float backHip = -frontHip;
    float frontKnee = -(8.0f + 26.0f * fmaxf(0.0f, cosf(frontPhase)));   // bend while the leg swings forward
    float backKnee = -(8.0f + 26.0f * fmaxf(0.0f, -cosf(frontPhase)));
    float armSwing = -cosf(pose.walkCycle) * 22.0f;

    // overhead throw (req 91): front arm winds up behind the head and whips forward over the top
    float frontRest = -2.0f + armSwing, backRest = 6.0f - armSwing;
    float leanRest = -4.0f + sinf(pose.time * 2.5f) * 1.5f;
    const float frontArmKeys[THROW_KEYS] = { frontRest, -150.0f, -265.0f, -320.0f, frontRest - 360.0f };
    const float frontElbowKeys[THROW_KEYS] = { 20.0f, 105.0f, 5.0f, 15.0f, 20.0f };
    const float backArmKeys[THROW_KEYS] = { backRest, 75.0f, -25.0f, -35.0f, backRest };   // other arm aims, then pulls back
    const float backElbowKeys[THROW_KEYS] = { 25.0f, 10.0f, 35.0f, 30.0f, 25.0f };
    const float leanKeys[THROW_KEYS] = { leanRest, 8.0f, -14.0f, -10.0f, leanRest };
    const float headKeys[THROW_KEYS] = { 0.0f, 6.0f, -8.0f, -4.0f, 0.0f };

    float frontArm = keyframes(a, THROW_TIMES, frontArmKeys, THROW_KEYS) + hit * 25.0f;
    float frontElbow = keyframes(a, THROW_TIMES, frontElbowKeys, THROW_KEYS);
    float backArm = keyframes(a, THROW_TIMES, backArmKeys, THROW_KEYS) + hit * 25.0f;
    float backElbow = keyframes(a, THROW_TIMES, backElbowKeys, THROW_KEYS);
    float lean = keyframes(a, THROW_TIMES, leanKeys, THROW_KEYS) + hit * 14.0f;   // hit: knocked back
    float headTilt = keyframes(a, THROW_TIMES, headKeys, THROW_KEYS) + hit * 12.0f;
    bool holdingStar = a < STRIKE_POINT;

    // mirror to face left, then sit the hips so the lowest foot is on the floor
    glm::mat4 root = translate2D(scale2D(base, -1.0f, 1.0f), -hit * 4.0f, 0.0f);
    float hipY = fmaxf(legDrop(frontHip, frontKnee), legDrop(backHip, backKnee));
    glm::mat4 hips = translate2D(root, 0.0f, hipY);
    glm::mat4 torso = joint(hips, 0.0f, 0.0f, lean);

    float flutter = sinf(pose.time * 6.0f + pose.walkCycle) * 8.0f;

    // back to front: far arm, far leg, near leg, torso, head, near arm, near shoulder plate
    drawArm(torso, -SHOULDER_X, backArm, backElbow, false, false, pose.time);
    Renderer::draw(pauldronMesh, part(torso, -6.0f, 37.0f, 0.9f, 0.9f), ARMOUR);
    drawLeg(hips, -3.0f, backHip, backKnee, false);
    drawLeg(hips, 3.0f, frontHip, frontKnee, true);
    drawTorso(torso, flutter);
    drawHead(torso, headTilt, flutter);
    drawArm(torso, SHOULDER_X, frontArm, frontElbow, true, holdingStar, pose.time);
    Renderer::draw(pauldronMesh, part(torso, 6.0f, 37.0f, 1.15f, 1.15f), ARMOUR_EDGE);
    Renderer::draw(pauldronMesh, part(torso, 6.0f, 37.0f, 1.0f, 1.0f), ARMOUR);
}
