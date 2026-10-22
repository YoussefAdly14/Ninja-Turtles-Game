#include "models/VillainModels.h"
#include "core/Renderer.h"
#include "core/Mesh.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "Config.h"
#include <cmath>
#include <vector>

// Bebop + Rocksteady (req 87, 89, 91, 123), init/shutdown and the per-kind dispatch.
// The Foot Soldier lives in FootModel.cpp.
// Both are drawn facing +x, then mirrored with scale2D(base, -1, 1) so they face left.
// Hierarchy = parent * child (learnopengl Transformations + Guest Articles Scene-Graph, Lab 4 orbiting cube):
//   hips -> torso -> shoulder -> elbow -> fist / blaster,   torso -> neck -> head -> horn / ears / tusks,
//   hips -> hip joint -> knee -> ankle -> boot.
// The two mutants share a colour family on purpose, so they must differ by SHAPE:
// Bebop = round ball body, short legs, mohawk spikes, blaster slung LOW near the floor;
// Rocksteady = biggest, wedge-shaped torso, horn, helmet, huge shoulder plates and fists.

const float PI = 3.14159265f;

// ---------- shared colours ----------
const glm::vec4 SKIN       = MUTANT_GRAY;
const glm::vec4 SKIN_DARK  = hexColor(0x555555);   // far limbs, creases
const glm::vec4 SKIN_LIGHT = hexColor(0x8A8484);   // belly, snout
const glm::vec4 IVORY      = hexColor(0xE6DDC6);   // tusks, horn
const glm::vec4 BOOT       = hexColor(0x2A2522);
const glm::vec4 SOLE       = hexColor(0x161210);
const glm::vec4 LEATHER    = hexColor(0x5A3A22);
const glm::vec4 BRASS      = hexColor(0xC09A44);
const glm::vec4 METAL      = hexColor(0x8C9198);
const glm::vec4 METAL_DARK = hexColor(0x50555D);

// Bebop
const glm::vec4 VEST       = hexColor(0x232227);   // black punk leather vest
const glm::vec4 VEST_TRIM  = hexColor(0xA8242E);
const glm::vec4 MOHAWK     = hexColor(0xB0287A);
const glm::vec4 MOHAWK_TIP = hexColor(0xD84A9C);
const glm::vec4 SHADES     = hexColor(0x1A1A24);
const glm::vec4 JEANS      = hexColor(0x3B4A6E);
const glm::vec4 JEANS_FAR  = hexColor(0x2B3752);
const glm::vec4 NOSE_PINK  = hexColor(0xC0948F);
const glm::vec4 GUN_BODY   = hexColor(0x464A53);
const glm::vec4 GUN_BARREL = hexColor(0x2C2F36);
const glm::vec4 BOLT_GLOW  = hexColor(0xFF5A1F);   // same orange as the blaster bolt (PropModels)
const glm::vec4 BOLT_CORE  = hexColor(0xFFD27A);

// Rocksteady
const glm::vec4 ARMOUR     = hexColor(0x3F454E);   // dark gunmetal plates, light rims: stand out from the gray skin
const glm::vec4 ARMOUR_RIM = hexColor(0xA3A9B2);
const glm::vec4 OLIVE      = hexColor(0x56603C);
const glm::vec4 OLIVE_DARK = hexColor(0x3C4429);
const glm::vec4 PANTS      = hexColor(0x4E4438);
const glm::vec4 PANTS_FAR  = hexColor(0x372F27);
const glm::vec4 EYE_YELLOW = hexColor(0xE8D27A);
const glm::vec4 DUST       = hexColor(0xB8A88C);

// ---------- sizes (model units = world units, feet at y = 0) ----------
const float BEBOP_THIGH = 18.0f, BEBOP_SHIN = 15.0f, BEBOP_BOOT = 12.0f;
const float BEBOP_UPPER = 30.0f, BEBOP_FORE = 28.0f;
const float BEBOP_STRIDE = 20.0f;

const float ROCK_THIGH = 32.0f, ROCK_SHIN = 28.0f, ROCK_BOOT = 15.0f;
const float ROCK_UPPER = 30.0f, ROCK_FORE = 28.0f;
const float ROCK_STRIDE = 30.0f;   // he runs: longer stride, deeper knee bend

// attack keys (attack progress 0..1). The shot / punch lands at STRIKE_POINT, the same frame
// Villain.cpp spawns the bolt or applies the melee hit.
const int   PUNCH_KEYS = 5;
const float PUNCH_TIMES[PUNCH_KEYS] = { 0.0f, STRIKE_POINT * 0.65f, STRIKE_POINT, STRIKE_POINT + 0.15f, 1.0f };

// meshes, built once in init()
static Mesh vestMesh, tuskMesh, shadesMesh, earMesh, gunMesh, flashMesh;
static Mesh rockTorsoMesh, rockVestMesh, rockHeadMesh, hornMesh, pauldronMesh, chestPlateMesh;

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

// how far below the hip the sole is; the hips sit on the lower leg so the feet stay on the floor
static float legDrop(float thigh, float shin, float boot, float hipDeg, float kneeDeg)
{
    return thigh * cosf(glm::radians(hipDeg)) + shin * cosf(glm::radians(hipDeg + kneeDeg)) + boot;
}

// leg angles from the walk phase: swing at the hip, bend the knee while the leg moves forward
static void legAngles(float phase, float stride, float kneeBend, float& hip, float& knee)
{
    hip = sinf(phase) * stride;
    knee = -(8.0f + kneeBend * fmaxf(0.0f, cosf(phase)));
}

// ============================== BEBOP ==============================

static void drawBebopLeg(const glm::mat4& hips, float x, float hipDeg, float kneeDeg, bool near)
{
    glm::vec4 cloth = near ? JEANS : JEANS_FAR;
    glm::mat4 hip = joint(hips, x, 0.0f, hipDeg);
    Renderer::draw(Shape::Quad, part(hip, 0.0f, -BEBOP_THIGH * 0.5f, 20.0f, BEBOP_THIGH + 6.0f), cloth);

    glm::mat4 knee = joint(hip, 0.0f, -BEBOP_THIGH, kneeDeg);
    Renderer::draw(Shape::Circle, part(knee, 0.0f, 0.0f, 18.0f, 18.0f), cloth);
    Renderer::draw(Shape::Quad, part(knee, 0.0f, -BEBOP_SHIN * 0.5f, 16.0f, BEBOP_SHIN), cloth);

    // chunky boot, kept flat
    glm::mat4 ankle = joint(knee, 0.0f, -BEBOP_SHIN, -(hipDeg + kneeDeg));
    Renderer::draw(Shape::Quad, part(ankle, 0.0f, 1.0f, 19.0f, 6.0f), near ? LEATHER : BOOT);   // cuff
    Renderer::draw(Shape::Quad, part(ankle, 3.0f, -BEBOP_BOOT * 0.5f, 24.0f, BEBOP_BOOT), BOOT);
    Renderer::draw(Shape::Circle, part(ankle, 14.0f, -BEBOP_BOOT * 0.5f + 0.5f, 12.0f, BEBOP_BOOT - 1.0f), BOOT);
    Renderer::draw(Shape::Quad, part(ankle, 5.0f, -BEBOP_BOOT + 1.5f, 30.0f, 3.0f), SOLE);
}

// far arm: hangs and swings, spiked wristband
static void drawBebopBackArm(const glm::mat4& torso, float shoulderDeg, float elbowDeg)
{
    glm::mat4 shoulder = joint(torso, -14.0f, 56.0f, shoulderDeg);
    Renderer::draw(Shape::Quad, part(shoulder, 0.0f, -13.0f, 15.0f, 30.0f), SKIN_DARK);
    glm::mat4 elbow = joint(shoulder, 0.0f, -26.0f, elbowDeg);
    Renderer::draw(Shape::Circle, part(elbow, 0.0f, 0.0f, 15.0f, 15.0f), SKIN_DARK);
    Renderer::draw(Shape::Quad, part(elbow, 0.0f, -12.0f, 14.0f, 24.0f), SKIN_DARK);
    Renderer::draw(Shape::Quad, part(elbow, 0.0f, -18.0f, 17.0f, 6.0f), LEATHER);   // wristband
    for (int i = -1; i <= 1; ++i)
        Renderer::draw(Shape::Triangle, part(joint(elbow, 0.0f, -18.0f, 90.0f + i * 35.0f), 0.0f, 10.0f, 3.5f, 5.0f), METAL);
    Renderer::draw(Shape::Circle, part(elbow, 0.0f, -27.0f, 17.0f, 16.0f), SKIN_DARK);   // fist
}

// near arm: long, drooping arm with the blaster strapped on, barrel aimed LOW (req 87, 123)
static void drawBebopGunArm(const glm::mat4& torso, float lean, float shoulderDeg, float elbowDeg,
                            float gunTilt, float charge, float muzzleFlash)
{
    // the arm comes out of the vest's arm hole; a dark rim behind each piece keeps it readable
    glm::mat4 shoulder = joint(torso, -2.0f, 56.0f, shoulderDeg);
    Renderer::draw(Shape::Quad, part(shoulder, 0.0f, -BEBOP_UPPER * 0.5f, 20.0f, BEBOP_UPPER + 6.0f), SKIN_DARK);
    Renderer::draw(Shape::Quad, part(shoulder, 0.0f, -BEBOP_UPPER * 0.5f, 16.0f, BEBOP_UPPER + 4.0f), SKIN);
    Renderer::draw(Shape::Circle, part(shoulder, 0.0f, -2.0f, 22.0f, 20.0f), SKIN);   // meaty shoulder
    // spiked leather band on the upper arm, spikes pointing back
    Renderer::draw(Shape::Quad, part(shoulder, 0.0f, -12.0f, 19.0f, 5.0f), LEATHER);
    for (int i = 0; i < 2; ++i)
        Renderer::draw(Shape::Triangle, part(joint(shoulder, -9.0f, -10.0f - i * 4.5f, 90.0f), 0.0f, 3.5f, 4.0f, 7.0f), METAL);

    glm::mat4 elbow = joint(shoulder, 0.0f, -BEBOP_UPPER, elbowDeg);
    Renderer::draw(Shape::Quad, part(elbow, 0.0f, -BEBOP_FORE * 0.5f, 19.0f, BEBOP_FORE + 2.0f), SKIN_DARK);
    Renderer::draw(Shape::Quad, part(elbow, 0.0f, -BEBOP_FORE * 0.5f, 15.0f, BEBOP_FORE), SKIN);
    Renderer::draw(Shape::Circle, part(elbow, 0.0f, 0.0f, 16.0f, 16.0f), SKIN);
    Renderer::draw(Shape::Quad, part(elbow, 0.0f, -BEBOP_FORE * 0.45f, 18.0f, 5.0f), LEATHER);   // gun strap

    // the blaster frame cancels the arm rotation so the barrel points forward, then tilts down
    glm::mat4 gun = joint(elbow, 0.0f, -BEBOP_FORE + 2.0f, -(lean + shoulderDeg + elbowDeg) + gunTilt);
    Renderer::draw(Shape::Quad, part(gun, -10.0f, 9.0f, 3.0f, 15.0f, 25.0f), GUN_BARREL);   // power cable
    Renderer::draw(gunMesh, gun, GUN_BODY);
    Renderer::draw(Shape::Quad, part(gun, 2.0f, 9.0f, 14.0f, 3.0f), METAL);               // top rail
    Renderer::draw(Shape::Quad, part(gun, 21.0f, -12.0f, 22.0f, 9.0f), GUN_BARREL);
    Renderer::draw(Shape::Quad, part(gun, 15.0f, -12.0f, 2.5f, 11.0f), METAL);            // cooling rings
    Renderer::draw(Shape::Quad, part(gun, 21.0f, -12.0f, 2.5f, 11.0f), METAL);
    Renderer::draw(Shape::Quad, part(gun, 32.0f, -12.0f, 4.0f, 13.0f), METAL);            // muzzle

    // energy cell glows brighter while charging the shot
    float glow = 0.55f + 0.45f * charge;
    Renderer::draw(Shape::Circle, part(gun, -2.0f, -6.0f, 10.0f, 10.0f), withAlpha(BOLT_GLOW, glow));
    Renderer::draw(Shape::Circle, part(gun, -2.0f, -6.0f, 4.0f + 2.0f * charge, 4.0f + 2.0f * charge), BOLT_CORE);

    if (muzzleFlash > 0.0f) {
        float s = 10.0f + 18.0f * muzzleFlash;
        Renderer::draw(flashMesh, part(gun, 36.0f, -12.0f, s * 1.4f, s, 0.0f), withAlpha(BOLT_GLOW, 0.8f));
        Renderer::draw(Shape::Circle, part(gun, 35.0f, -12.0f, s * 0.6f, s * 0.45f), BOLT_CORE);
    }
}

static void drawBebopBody(const glm::mat4& torso, float breathe)
{
    Renderer::draw(Shape::Circle, part(torso, -2.0f, 32.0f, 84.0f, 76.0f), SKIN);                         // round hulk
    Renderer::draw(Shape::Circle, part(torso, 14.0f, 22.0f, 54.0f, 54.0f * breathe), SKIN_LIGHT);         // belly
    Renderer::draw(Shape::Quad, part(torso, 16.0f, 22.0f, 2.0f, 3.0f), SKIN_DARK);                        // belly button
    // sleeveless punk vest: straight hem, red trim, studs along the hem and back
    Renderer::draw(vestMesh, torso, VEST);
    Renderer::draw(Shape::Quad, part(torso, 4.0f, 37.0f, 4.0f, 47.0f, -25.5f), VEST_TRIM);
    Renderer::draw(Shape::Quad, part(torso, -23.0f, 21.0f, 37.0f, 3.0f), VEST_TRIM);                     // hem
    Renderer::draw(Shape::Triangle, part(torso, 8.0f, 60.0f, 10.0f, 12.0f, 160.0f), VEST_TRIM);           // collar point
    for (int i = 0; i < 4; ++i)
        Renderer::draw(Shape::Circle, part(torso, -36.0f + i * 7.0f, 25.0f, 3.5f, 3.5f), METAL);
    Renderer::draw(Shape::Circle, part(torso, -38.0f, 36.0f, 3.5f, 3.5f), METAL);
    Renderer::draw(Shape::Circle, part(torso, -34.0f, 49.0f, 3.5f, 3.5f), METAL);
    Renderer::draw(Shape::Circle, part(torso, -25.0f, 59.0f, 3.5f, 3.5f), METAL);
    // belt under the belly
    Renderer::draw(Shape::Quad, part(torso, 0.0f, 3.0f, 58.0f, 8.0f, 6.0f), hexColor(0x2E2018));
    Renderer::draw(Shape::Quad, part(torso, 24.0f, 6.0f, 10.0f, 10.0f, 6.0f), BRASS);
}

static void drawBebopHead(const glm::mat4& torso, float tilt, float earWiggle)
{
    glm::mat4 neck = joint(torso, 18.0f, 56.0f, tilt);
    glm::mat4 head = translate2D(neck, 6.0f, 6.0f);

    // far ear, mohawk (its base hides under the skull), far tusk
    Renderer::draw(earMesh, part(joint(head, -4.0f, 16.0f, 20.0f + earWiggle), 0.0f, 0.0f, 1.0f, 1.0f), SKIN_DARK);
    for (int i = 0; i < 6; ++i) {
        glm::mat4 spike = joint(head, 0.0f, 0.0f, -25.0f + i * 20.0f);
        float h = (i % 2 == 0) ? 20.0f : 15.0f;
        Renderer::draw(Shape::Triangle, part(spike, 0.0f, 17.0f + h * 0.5f, 10.0f, h), (i % 2 == 0) ? MOHAWK : MOHAWK_TIP);
    }
    Renderer::draw(tuskMesh, part(head, 16.0f, -15.0f, 0.8f, 0.8f, -5.0f), hexColor(0xBDB49E));

    Renderer::draw(Shape::Circle, part(head, 0.0f, 0.0f, 44.0f, 40.0f), SKIN);                     // skull
    Renderer::draw(Shape::Circle, part(head, 12.0f, -14.0f, 30.0f, 12.0f), SKIN_DARK);             // lower jaw
    Renderer::draw(Shape::Circle, part(head, 21.0f, -5.0f, 28.0f, 20.0f), SKIN_LIGHT);             // snout
    Renderer::draw(Shape::Circle, part(head, 34.0f, -4.0f, 8.0f, 16.0f), NOSE_PINK);               // flat pig nose
    Renderer::draw(Shape::Circle, part(head, 35.5f, -1.0f, 2.5f, 3.5f), SKIN_DARK);                 // nostrils
    Renderer::draw(Shape::Circle, part(head, 35.5f, -7.0f, 2.5f, 3.5f), SKIN_DARK);
    Renderer::draw(Shape::Quad, part(head, 20.0f, -11.5f, 15.0f, 2.0f, -10.0f), SKIN_DARK);        // grin
    Renderer::draw(tuskMesh, part(head, 25.0f, -15.0f, 1.0f, 1.0f, -12.0f), IVORY);

    // wraparound shades + strap + glint, a heavy brow on top
    Renderer::draw(Shape::Quad, part(head, -7.0f, 5.0f, 26.0f, 3.0f), SHADES);
    Renderer::draw(shadesMesh, part(head, 12.0f, 4.0f, 1.0f, 1.0f), SHADES);
    Renderer::draw(Shape::Quad, part(head, 17.0f, 6.0f, 6.0f, 1.5f, 20.0f), hexColor(0x7FA0C8));
    Renderer::draw(Shape::Quad, part(head, 12.0f, 10.5f, 16.0f, 3.0f, -8.0f), SKIN_DARK);

    // near ear with an earring
    glm::mat4 ear = joint(head, -9.0f, 13.0f, 35.0f + earWiggle);
    Renderer::draw(earMesh, ear, SKIN);
    Renderer::draw(earMesh, part(ear, 0.0f, 2.0f, 0.55f, 0.55f), hexColor(0xB07A7A));
    Renderer::draw(Shape::Ring, part(head, -12.0f, 6.0f, 7.0f, 7.0f), BRASS);
}

void VillainModels::drawBebop(const glm::mat4& base, const Pose& pose)
{
    float hit = pose.hitFlash;
    float a = pose.attack;

    // waddle: short stomping legs, body rolls side to side
    float frontPhase = pose.walkCycle + PI * 0.5f;
    float frontHip, frontKnee, backHip, backKnee;
    legAngles(frontPhase, BEBOP_STRIDE, 22.0f, frontHip, frontKnee);
    legAngles(frontPhase + PI, BEBOP_STRIDE, 22.0f, backHip, backKnee);
    float roll = sinf(pose.walkCycle) * 3.0f;

    // shot (req 91): aim the barrel down while the cell charges, then recoil on STRIKE_POINT
    float charge = (a > 0.0f && a < STRIKE_POINT) ? a / STRIKE_POINT : 0.0f;
    float kick = 0.0f;
    if (a >= STRIKE_POINT) {
        float k = (a - STRIKE_POINT) / (1.0f - STRIKE_POINT);
        kick = (k < 0.1f) ? k / 0.1f : (1.0f - k) * (1.0f - k) / 0.81f;
    }
    float muzzleFlash = (a >= STRIKE_POINT && a < STRIKE_POINT + 0.2f) ? 1.0f - (a - STRIKE_POINT) / 0.2f : 0.0f;

    float lean = -6.0f + roll - easeInOut(charge) * 3.0f + kick * 8.0f + hit * 10.0f;
    float gunArm = 14.0f - cosf(pose.walkCycle) * 6.0f - kick * 14.0f + hit * 10.0f;
    float gunElbow = 20.0f + kick * 10.0f;
    float gunTilt = -12.0f - easeInOut(charge) * 4.0f + kick * 20.0f;
    float backArm = 8.0f + cosf(pose.walkCycle) * 18.0f + hit * 20.0f;
    float headTilt = sinf(pose.time * 5.0f) * 2.0f + kick * 6.0f + hit * 14.0f;   // idle snicker bob
    float breathe = 1.0f + 0.03f * sinf(pose.time * 2.5f);

    glm::mat4 root = translate2D(scale2D(base, -1.0f, 1.0f), -kick * 4.0f - hit * 5.0f, 0.0f);
    float hipY = fmaxf(legDrop(BEBOP_THIGH, BEBOP_SHIN, BEBOP_BOOT, frontHip, frontKnee),
                       legDrop(BEBOP_THIGH, BEBOP_SHIN, BEBOP_BOOT, backHip, backKnee));
    glm::mat4 hips = translate2D(root, 0.0f, hipY);
    glm::mat4 torso = joint(hips, 0.0f, 0.0f, lean);

    drawBebopBackArm(torso, backArm, 30.0f);
    drawBebopLeg(hips, -12.0f, backHip, backKnee, false);
    drawBebopLeg(hips, 10.0f, frontHip, frontKnee, true);
    drawBebopBody(torso, breathe);
    drawBebopHead(torso, headTilt, sinf(pose.time * 4.0f) * 4.0f);
    drawBebopGunArm(torso, lean, gunArm, gunElbow, gunTilt, charge, muzzleFlash);
}

// ============================== ROCKSTEADY ==============================

static void drawRockLeg(const glm::mat4& hips, float x, float hipDeg, float kneeDeg, bool near)
{
    glm::vec4 cloth = near ? PANTS : PANTS_FAR;
    glm::mat4 hip = joint(hips, x, 0.0f, hipDeg);
    Renderer::draw(Shape::Quad, part(hip, 0.0f, -ROCK_THIGH * 0.5f, 25.0f, ROCK_THIGH + 6.0f), cloth);

    glm::mat4 knee = joint(hip, 0.0f, -ROCK_THIGH, kneeDeg);
    Renderer::draw(Shape::Quad, part(knee, 0.0f, -ROCK_SHIN * 0.5f, 21.0f, ROCK_SHIN), cloth);
    // angular knee guard
    Renderer::draw(chestPlateMesh, part(knee, 4.0f, 0.0f, 1.0f, 0.9f, -(hipDeg + kneeDeg) * 0.5f), near ? ARMOUR_RIM : ARMOUR);
    Renderer::draw(chestPlateMesh, part(knee, 4.0f, 0.0f, 0.82f, 0.72f, -(hipDeg + kneeDeg) * 0.5f), ARMOUR);

    glm::mat4 ankle = joint(knee, 0.0f, -ROCK_SHIN, -(hipDeg + kneeDeg));
    Renderer::draw(Shape::Quad, part(ankle, 0.0f, 1.0f, 24.0f, 7.0f), cloth);   // trouser tuck
    Renderer::draw(Shape::Quad, part(ankle, 5.0f, -ROCK_BOOT * 0.5f, 32.0f, ROCK_BOOT), BOOT);
    Renderer::draw(Shape::Circle, part(ankle, 19.0f, -ROCK_BOOT * 0.5f + 0.5f, 14.0f, ROCK_BOOT - 1.0f), BOOT);
    Renderer::draw(Shape::Quad, part(ankle, 7.0f, -ROCK_BOOT + 1.5f, 38.0f, 3.5f), SOLE);
}

static void drawRockArm(const glm::mat4& torso, float x, float y, float shoulderDeg, float elbowDeg, bool near)
{
    glm::vec4 skin = near ? SKIN : SKIN_DARK;
    glm::mat4 shoulder = joint(torso, x, y, shoulderDeg);
    Renderer::draw(Shape::Quad, part(shoulder, 0.0f, -ROCK_UPPER * 0.5f, 21.0f, ROCK_UPPER + 6.0f), skin);

    glm::mat4 elbow = joint(shoulder, 0.0f, -ROCK_UPPER, elbowDeg);
    Renderer::draw(Shape::Circle, part(elbow, 0.0f, 0.0f, 20.0f, 20.0f), skin);
    Renderer::draw(Shape::Quad, part(elbow, 0.0f, -ROCK_FORE * 0.5f, 20.0f, ROCK_FORE), skin);
    Renderer::draw(Shape::Quad, part(elbow, 0.0f, -ROCK_FORE * 0.6f, 24.0f, 14.0f), near ? ARMOUR_RIM : ARMOUR);   // bracer
    Renderer::draw(Shape::Quad, part(elbow, 0.0f, -ROCK_FORE * 0.6f, 20.0f, 10.0f), ARMOUR);

    // big fist: knuckles + thumb, a bit lighter so it reads against the body
    glm::mat4 fist = translate2D(elbow, 0.0f, -ROCK_FORE - 7.0f);
    glm::vec4 fistColor = near ? hexColor(0x7C7878) : SKIN_DARK;
    Renderer::draw(Shape::Circle, part(fist, 0.0f, 0.0f, 29.0f, 27.0f), fistColor);
    for (int i = 0; i < 3; ++i)
        Renderer::draw(Shape::Circle, part(fist, -8.0f + i * 8.0f, -11.0f, 9.0f, 8.0f), fistColor);
    Renderer::draw(Shape::Quad, part(fist, 0.0f, -4.0f, 22.0f, 2.0f), SKIN_DARK);   // finger crease
    Renderer::draw(Shape::Circle, part(fist, 11.0f, 4.0f, 10.0f, 12.0f), skin);     // thumb
}

static void drawPauldron(const glm::mat4& torso, float x, float y, bool near)
{
    glm::mat4 m = translate2D(torso, x, y);
    glm::vec4 plate = near ? ARMOUR : SOLE;
    glm::vec4 rim = near ? ARMOUR_RIM : ARMOUR;
    if (near) {
        Renderer::draw(Shape::Triangle, part(joint(m, -4.0f, 10.0f, 20.0f), 0.0f, 5.0f, 7.0f, 13.0f), IVORY);   // spikes
        Renderer::draw(Shape::Triangle, part(joint(m, 8.0f, 10.0f, -10.0f), 0.0f, 5.0f, 7.0f, 13.0f), IVORY);
    }
    Renderer::draw(pauldronMesh, part(m, 3.0f, -16.0f, 0.85f, 0.75f), rim);   // lower plate
    Renderer::draw(pauldronMesh, part(m, 3.0f, -16.0f, 0.72f, 0.62f), plate);
    Renderer::draw(pauldronMesh, part(m, 0.0f, 0.0f, 1.14f, 1.14f), rim);      // main plate + rim
    Renderer::draw(pauldronMesh, m, plate);
    if (near) {
        for (int i = 0; i < 3; ++i)
            Renderer::draw(Shape::Circle, part(m, -8.0f + i * 8.0f, 3.0f - i * 0.5f, 3.5f, 3.5f), IVORY);
    }
}

static void drawRockTorso(const glm::mat4& torso)
{
    Renderer::draw(rockTorsoMesh, torso, SKIN);
    Renderer::draw(rockVestMesh, torso, OLIVE);
    Renderer::draw(chestPlateMesh, part(torso, 16.0f, 34.0f, 1.14f, 1.12f), ARMOUR_RIM);
    Renderer::draw(chestPlateMesh, part(torso, 16.0f, 34.0f, 1.0f, 1.0f), ARMOUR);

    // ammo strap across the chest
    Renderer::draw(Shape::Quad, part(torso, 2.0f, 32.0f, 7.0f, 68.0f, -40.0f), LEATHER);
    for (int i = 0; i < 4; ++i) {
        float t = 0.25f + i * 0.15f;
        Renderer::draw(Shape::Quad, part(torso, -20.0f + 44.0f * t, 6.0f + 52.0f * t, 4.0f, 8.0f, -40.0f), BRASS);
    }

    // belt, buckle, pouches
    Renderer::draw(Shape::Quad, part(torso, 0.0f, 1.0f, 50.0f, 10.0f), LEATHER);
    Renderer::draw(Shape::Quad, part(torso, -14.0f, -5.0f, 11.0f, 11.0f), OLIVE_DARK);
    Renderer::draw(Shape::Quad, part(torso, -14.0f, -1.0f, 12.0f, 3.0f), OLIVE);
    Renderer::draw(Shape::Quad, part(torso, 19.0f, 1.0f, 10.0f, 10.0f), BRASS);
}

static void drawRockHead(const glm::mat4& torso, float tilt, float snort)
{
    glm::mat4 neck = joint(torso, 13.0f, 66.0f, tilt);
    Renderer::draw(Shape::Quad, part(neck, 2.0f, 4.0f, 24.0f, 18.0f, -20.0f), SKIN);   // thick neck
    glm::mat4 head = translate2D(neck, 8.0f, 8.0f);

    glm::mat4 ear = joint(head, -10.0f, 14.0f, 40.0f + snort * 3.0f);
    Renderer::draw(earMesh, ear, SKIN);
    Renderer::draw(earMesh, part(ear, 0.0f, 2.0f, 0.55f, 0.55f), hexColor(0x9A7A7A));

    Renderer::draw(rockHeadMesh, head, SKIN);
    Renderer::draw(Shape::Quad, part(head, 24.0f, 4.0f, 2.0f, 12.0f, 10.0f), SKIN_DARK);   // skin folds
    Renderer::draw(Shape::Quad, part(head, 30.0f, 0.0f, 2.0f, 10.0f, 10.0f), SKIN_DARK);
    Renderer::draw(Shape::Quad, part(head, 32.0f, -9.0f, 18.0f, 2.5f, 8.0f), SKIN_DARK);  // mouth
    Renderer::draw(Shape::Circle, part(head, 41.0f, -1.0f, 3.5f, 4.5f), SOLE);            // nostril

    // horns: the big one on the snout, a small one behind it
    Renderer::draw(hornMesh, part(head, 20.0f, 15.0f, 0.5f, 0.5f, -5.0f), IVORY);
    Renderer::draw(hornMesh, part(head, 34.0f, 8.0f, 0.95f, 0.95f, -18.0f), IVORY);

    // small mean eye under a heavy brow
    Renderer::draw(Shape::Circle, part(head, 12.0f, 5.0f, 7.0f, 6.0f), EYE_YELLOW);
    Renderer::draw(Shape::Circle, part(head, 13.5f, 5.0f, 3.0f, 3.5f), SOLE);
    Renderer::draw(Shape::Quad, part(head, 12.0f, 9.5f, 14.0f, 4.0f, -15.0f), SKIN_DARK);

    // army helmet: dome, rim, rivets, chin strap
    glm::mat4 helmet = joint(head, -2.0f, 15.0f, 8.0f);
    Renderer::draw(Shape::Quad, part(helmet, -6.0f, -11.0f, 3.0f, 22.0f, -10.0f), LEATHER);
    Renderer::draw(Shape::Semicircle, part(helmet, 0.0f, 0.0f, 36.0f, 30.0f), OLIVE);
    Renderer::draw(Shape::Quad, part(helmet, 0.0f, 1.0f, 42.0f, 5.0f), OLIVE_DARK);
    Renderer::draw(Shape::Circle, part(helmet, -6.0f, 7.0f, 3.0f, 3.0f), METAL);
    Renderer::draw(Shape::Circle, part(helmet, 6.0f, 7.0f, 3.0f, 3.0f), METAL);
}

// dust kicked up when a foot lands; phase = that leg's walk phase. Invisible at walkCycle 0 (idle).
static void drawDustPuff(const glm::mat4& root, float phase, float x)
{
    const float landing = 0.6f * PI;   // the foot hits the floor here
    const float life = 0.8f * PI;      // how long (in phase) the puff lasts
    float u = fmodf(phase - landing, 2.0f * PI);
    if (u < 0.0f) u += 2.0f * PI;
    float t = u / life;
    if (t >= 1.0f) return;

    glm::vec4 c = withAlpha(DUST, 0.75f * (1.0f - t * t));
    float px = x - t * 40.0f, py = 6.0f + t * 10.0f;
    Renderer::draw(Shape::Circle, part(root, px, py, 18.0f + 20.0f * t, 14.0f + 14.0f * t), c);
    Renderer::draw(Shape::Circle, part(root, px - 10.0f - 6.0f * t, py + 3.0f, 8.0f + 12.0f * t, 7.0f + 9.0f * t), c);
    Renderer::draw(Shape::Circle, part(root, px + 8.0f, py + 5.0f + 4.0f * t, 6.0f + 10.0f * t, 6.0f + 8.0f * t), c);
}

void VillainModels::drawRocksteady(const glm::mat4& base, const Pose& pose)
{
    float hit = pose.hitFlash;
    float a = pose.attack;

    // run cycle (he is the fast one): long stride, deep knee bend, arms pumping
    float frontPhase = pose.walkCycle + PI * 0.5f;
    float frontHip, frontKnee, backHip, backKnee;
    legAngles(frontPhase, ROCK_STRIDE, 45.0f, frontHip, frontKnee);
    legAngles(frontPhase + PI, ROCK_STRIDE, 45.0f, backHip, backKnee);
    float pump = cosf(pose.walkCycle) * 25.0f;
    float snort = sinf(pose.time * 2.0f);

    // charging punch + horn butt (req 91): cock the fist back, lunge, the punch lands on STRIKE_POINT
    float frontRest = 5.0f - pump, backRest = 30.0f + pump;
    float leanRest = -7.0f + sinf(pose.walkCycle * 2.0f) * 2.0f;
    const float frontArmKeys[PUNCH_KEYS] = { frontRest, -45.0f, 88.0f, 82.0f, frontRest };
    const float frontElbowKeys[PUNCH_KEYS] = { 75.0f, 120.0f, 4.0f, 8.0f, 75.0f };
    const float backArmKeys[PUNCH_KEYS] = { backRest, 45.0f, -35.0f, -30.0f, backRest };
    const float backElbowKeys[PUNCH_KEYS] = { 75.0f, 80.0f, 100.0f, 95.0f, 75.0f };
    const float leanKeys[PUNCH_KEYS] = { leanRest, -4.0f, -30.0f, -26.0f, leanRest };
    const float headKeys[PUNCH_KEYS] = { -4.0f, 10.0f, -16.0f, -12.0f, -4.0f };
    const float lungeKeys[PUNCH_KEYS] = { 0.0f, -5.0f, 14.0f, 12.0f, 0.0f };

    float frontArm = keyframes(a, PUNCH_TIMES, frontArmKeys, PUNCH_KEYS) + hit * 15.0f;
    float frontElbow = keyframes(a, PUNCH_TIMES, frontElbowKeys, PUNCH_KEYS);
    float backArm = keyframes(a, PUNCH_TIMES, backArmKeys, PUNCH_KEYS) + hit * 15.0f;
    float backElbow = keyframes(a, PUNCH_TIMES, backElbowKeys, PUNCH_KEYS);
    float lean = keyframes(a, PUNCH_TIMES, leanKeys, PUNCH_KEYS) + hit * 8.0f;   // tough: small flinch
    float headTilt = keyframes(a, PUNCH_TIMES, headKeys, PUNCH_KEYS) + snort * 1.5f + hit * 10.0f;
    float lunge = keyframes(a, PUNCH_TIMES, lungeKeys, PUNCH_KEYS) - hit * 3.0f;

    glm::mat4 root = scale2D(base, -1.0f, 1.0f);
    // dust stays where it was kicked up (drawn before the lunge offset), behind everything
    drawDustPuff(root, frontPhase, 26.0f);
    drawDustPuff(root, frontPhase + PI, 12.0f);

    glm::mat4 body = translate2D(root, lunge - 6.0f, 0.0f);   // -6: keeps the big head + horn close to his hitbox
    float hipY = fmaxf(legDrop(ROCK_THIGH, ROCK_SHIN, ROCK_BOOT, frontHip, frontKnee),
                       legDrop(ROCK_THIGH, ROCK_SHIN, ROCK_BOOT, backHip, backKnee));
    glm::mat4 hips = translate2D(body, 0.0f, hipY);
    glm::mat4 torso = joint(hips, 0.0f, 0.0f, lean);

    drawRockArm(torso, -14.0f, 60.0f, backArm, backElbow, false);
    drawPauldron(torso, -16.0f, 62.0f, false);
    drawRockLeg(hips, -10.0f, backHip, backKnee, false);
    drawRockLeg(hips, 10.0f, frontHip, frontKnee, true);
    drawRockTorso(torso);
    drawRockHead(torso, headTilt, snort);
    drawRockArm(torso, 12.0f, 58.0f, frontArm, frontElbow, true);
    drawPauldron(torso, 8.0f, 58.0f, true);
}

// ============================== init / dispatch ==============================

void VillainModels::init()
{
    initFootSoldier();

    // Bebop's open vest covers his back and shoulders, the belly sticks out in front (torso coords)
    // (flush with the round back, straight hem high up so his gray backside shows below it)
    vestMesh = createPolygonMesh({ { -4.0f, 20.0f }, { -41.5f, 20.0f }, { -43.4f, 25.4f }, { -43.9f, 32.0f }, { -43.4f, 38.6f },
                                   { -38.4f, 51.0f }, { -29.0f, 61.1f }, { -16.4f, 67.7f }, { -5.7f, 69.9f }, { 6.0f, 69.0f },
                                   { 14.0f, 58.0f }, { 4.0f, 37.0f } });
    // curved tusk, base at (0,0), tip curling back
    tuskMesh = createPolygonMesh({ { -3.0f, 0.0f }, { 3.5f, 0.0f }, { 4.5f, 6.0f }, { 2.5f, 12.0f }, { -2.0f, 16.0f }, { -1.0f, 8.0f } });
    shadesMesh = createPolygonMesh({ { -4.0f, 3.0f }, { 12.0f, 5.5f }, { 17.0f, 2.0f }, { 15.0f, -3.5f },
                                     { 8.0f, -4.5f }, { 1.0f, -3.0f }, { -4.0f, -1.0f } });
    // pointy ear, base at (0,0), pointing up
    earMesh = createPolygonMesh({ { -5.0f, 0.0f }, { 5.0f, 0.0f }, { 5.0f, 8.0f }, { 1.0f, 17.0f }, { -4.0f, 9.0f } });
    // blaster housing around the fist (gun frame: +x = barrel direction)
    gunMesh = createPolygonMesh({ { -10.0f, 8.0f }, { 6.0f, 10.0f }, { 12.0f, 4.0f }, { 13.0f, -10.0f },
                                  { 7.0f, -19.0f }, { -6.0f, -18.0f }, { -12.0f, -6.0f } });
    // muzzle flash: 8-point star
    std::vector<glm::vec2> star;
    for (int i = 0; i < 16; ++i) {
        float ang = PI * 2.0f * i / 16.0f;
        float r = (i % 2 == 0) ? 0.5f : 0.22f;
        star.push_back(glm::vec2(r * cosf(ang), r * sinf(ang)));
    }
    flashMesh = createPolygonMesh(star);

    // Rocksteady: wedge torso, much wider at the shoulders than at the waist
    rockTorsoMesh = createPolygonMesh({ { -22.0f, -4.0f }, { 20.0f, -4.0f }, { 28.0f, 28.0f }, { 36.0f, 52.0f }, { 28.0f, 66.0f },
                                        { -18.0f, 70.0f }, { -34.0f, 58.0f }, { -30.0f, 26.0f } });
    rockVestMesh = createPolygonMesh({ { -22.0f, -2.0f }, { 20.0f, -2.0f }, { 27.0f, 28.0f }, { 32.0f, 50.0f },
                                       { 14.0f, 56.0f }, { -20.0f, 58.0f }, { -31.0f, 28.0f } });
    // long rhino head in profile, skull at (0,0), snout to +x
    rockHeadMesh = createPolygonMesh({ { -14.0f, -6.0f }, { -14.0f, 8.0f }, { -6.0f, 18.0f }, { 10.0f, 20.0f }, { 24.0f, 14.0f },
                                       { 36.0f, 6.0f }, { 43.0f, -2.0f }, { 41.0f, -10.0f }, { 26.0f, -15.0f }, { 4.0f, -15.0f } });
    // horn: wide base at (0,0), curving back to a point
    hornMesh = createPolygonMesh({ { -8.0f, -1.0f }, { 8.0f, -1.0f }, { 8.0f, 10.0f }, { 4.0f, 22.0f }, { -3.0f, 32.0f },
                                   { -3.0f, 21.0f }, { -6.0f, 9.0f } });
    pauldronMesh = createPolygonMesh({ { -16.0f, 6.0f }, { -6.0f, 13.0f }, { 8.0f, 13.0f }, { 18.0f, 6.0f },
                                       { 19.0f, -6.0f }, { 10.0f, -12.0f }, { -6.0f, -12.0f }, { -16.0f, -5.0f } });
    chestPlateMesh = createPolygonMesh({ { -10.0f, -12.0f }, { 8.0f, -12.0f }, { 12.0f, 0.0f }, { 10.0f, 12.0f },
                                         { -8.0f, 14.0f }, { -13.0f, 2.0f } });
}

void VillainModels::shutdown()
{
    shutdownFootSoldier();
    destroyMesh(vestMesh);
    destroyMesh(tuskMesh);
    destroyMesh(shadesMesh);
    destroyMesh(earMesh);
    destroyMesh(gunMesh);
    destroyMesh(flashMesh);
    destroyMesh(rockTorsoMesh);
    destroyMesh(rockVestMesh);
    destroyMesh(rockHeadMesh);
    destroyMesh(hornMesh);
    destroyMesh(pauldronMesh);
    destroyMesh(chestPlateMesh);
}

void VillainModels::draw(VillainKind kind, const glm::mat4& base, const Pose& pose)
{
    switch (kind) {
    case VillainKind::FootSoldier: drawFootSoldier(base, pose); break;
    case VillainKind::Bebop:       drawBebop(base, pose);       break;
    case VillainKind::Rocksteady:  drawRocksteady(base, pose);  break;
    default: break;
    }
}
