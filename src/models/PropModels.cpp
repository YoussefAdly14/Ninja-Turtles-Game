#include "models/PropModels.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "Config.h"
#include <cmath>
#include <vector>

// ---------------- pizza slice (req 104) ----------------
// Built once in init() in "slice units" (1 = PIZZA_SIZE) and scaled when drawn. A slice is a pie
// sector, so every piece is a triangle fan from the tip (Lab 3 circle pattern) with per-vertex colours.
static Mesh g_pizzaOutline;   // dark silhouette so the slice pops on any background
static Mesh g_pizzaSauce;     // thin red band peeking out under the cheese
static Mesh g_pizzaCheese;    // wavy melted edge, brighter in the middle
static Mesh g_pizzaCrust;     // curved crust band, golden inside -> baked brown outside (EBO)

const float PIZZA_TIP_Y = -0.5f;
const float PIZZA_HALF_ANGLE = 28.0f;    // degrees either side of straight up
const float PIZZA_PI = 3.14159265f;

// fan from the tip to an arc of radius r(i), one colour at the tip and one on the arc
static Mesh createSliceFan(float halfAngleDeg, int segments, float radius, float wobble,
                           glm::vec3 tipColor, glm::vec3 rimColor, float tipDrop)
{
    std::vector<float> v = { 0.0f, PIZZA_TIP_Y - tipDrop, 0.0f, tipColor.r, tipColor.g, tipColor.b };
    for (int i = 0; i <= segments; ++i) {
        float a = glm::radians(90.0f + halfAngleDeg - 2.0f * halfAngleDeg * i / segments);
        float r = radius + wobble * sinf(i * 2.3f);
        v.insert(v.end(), { r * cosf(a), PIZZA_TIP_Y + r * sinf(a), 0.0f, rimColor.r, rimColor.g, rimColor.b });
    }
    return createMesh(v.data(), v.size() * sizeof(float), GL_TRIANGLE_FAN);
}

void PropModels::init()
{
    g_pizzaOutline = createSliceFan(PIZZA_HALF_ANGLE + 3.0f, 16, 1.12f, 0.0f, glm::vec3(1.0f), glm::vec3(1.0f), 0.07f);
    g_pizzaSauce = createSliceFan(PIZZA_HALF_ANGLE + 0.5f, 16, 0.93f, 0.0f, glm::vec3(hexColor(0xB8452A)), glm::vec3(hexColor(0x9E3320)), 0.0f);
    g_pizzaCheese = createSliceFan(PIZZA_HALF_ANGLE - 1.5f, 16, 0.86f, 0.025f, glm::vec3(hexColor(0xFCE38E)), glm::vec3(PIZZA), 0.0f);

    // crust: band between two arcs (2 vertices per step + EBO, like the ring mesh), slightly lumpy outside
    std::vector<float> v;
    std::vector<unsigned int> idx;
    const int segments = 16;
    glm::vec3 inner(hexColor(0xE5A853)), outer(hexColor(0x9C5A22));
    for (int i = 0; i <= segments; ++i) {
        float a = glm::radians(90.0f + (PIZZA_HALF_ANGLE + 1.5f) * (1.0f - 2.0f * i / segments));
        float ro = 1.06f + 0.025f * sinf(i * 1.9f);
        v.insert(v.end(), { 0.86f * cosf(a), PIZZA_TIP_Y + 0.86f * sinf(a), 0.0f, inner.r, inner.g, inner.b });
        v.insert(v.end(), { ro * cosf(a), PIZZA_TIP_Y + ro * sinf(a), 0.0f, outer.r, outer.g, outer.b });
    }
    for (unsigned int i = 0; i < static_cast<unsigned int>(segments); ++i) {
        unsigned int a = 2 * i;
        idx.insert(idx.end(), { a, a + 2, a + 1,   a + 1, a + 2, a + 3 });
    }
    g_pizzaCrust = createIndexedMesh(v.data(), v.size() * sizeof(float), idx.data(), idx.size() * sizeof(unsigned int));
}

void PropModels::shutdown()
{
    destroyMesh(g_pizzaOutline);
    destroyMesh(g_pizzaSauce);
    destroyMesh(g_pizzaCheese);
    destroyMesh(g_pizzaCrust);
}

// Pizza slice (req 104): outline, sauce, melted cheese, crust with bubbles, three pepperoni with
// grease spots, basil bits, a stretching cheese drip and a shine. Tip down, bobbing and rocking.
// Used for the dropped collectible and the HUD coin icon (time 0 = still).
void PropModels::drawPizza(const glm::mat4& base, float time)
{
    glm::mat4 m = translate2D(base, 0.0f, sinf(time * 3.0f) * 4.0f);   // bob
    m = rotate2D(m, 5.0f * sinf(time * 1.7f));                           // gentle rock
    m = scale2D(m, PIZZA_SIZE, PIZZA_SIZE);                              // slice units -> world units

    Renderer::draw(g_pizzaOutline, m, glm::vec4(0.16f, 0.07f, 0.02f, 0.85f));
    Renderer::draw(g_pizzaSauce, m, WHITE);
    Renderer::draw(g_pizzaCheese, m, WHITE);
    Renderer::draw(g_pizzaCrust, m, WHITE);

    // crust bubbles + a toasted line
    Renderer::draw(Shape::Circle, part(m, -0.2f, 0.42f, 0.1f, 0.06f, -12.0f), hexColor(0x7E4518));
    Renderer::draw(Shape::Circle, part(m, 0.17f, 0.44f, 0.08f, 0.05f, 12.0f), hexColor(0x7E4518));
    Renderer::draw(Shape::Circle, part(m, 0.02f, 0.52f, 0.22f, 0.04f), hexColor(0xF2C27A, 0.7f));

    // pepperoni: dark rim, slice, grease spots, shine
    const float peps[3][3] = { { -0.11f, 0.08f, 0.1f }, { 0.14f, 0.17f, 0.095f }, { 0.01f, -0.17f, 0.08f } };
    for (const auto& p : peps) {
        Renderer::draw(Shape::Circle, part(m, p[0], p[1], 2.0f * p[2], 2.0f * p[2]), hexColor(0x7E1A14));
        Renderer::draw(Shape::Circle, part(m, p[0], p[1], 1.7f * p[2], 1.7f * p[2]), hexColor(0xB3261E));
        Renderer::draw(Shape::Circle, part(m, p[0] + 0.3f * p[2], p[1] - 0.3f * p[2], 0.35f * p[2], 0.35f * p[2]), hexColor(0x8E1D16));
        Renderer::draw(Shape::Circle, part(m, p[0] - 0.35f * p[2], p[1] + 0.35f * p[2], 0.45f * p[2], 0.3f * p[2]), withAlpha(WHITE, 0.35f));
    }
    // basil bits
    Renderer::draw(Shape::Quad, part(m, 0.12f, -0.02f, 0.07f, 0.035f, 30.0f), hexColor(0x3E7D2A));
    Renderer::draw(Shape::Quad, part(m, -0.15f, 0.28f, 0.06f, 0.03f, -20.0f), hexColor(0x3E7D2A));

    // cheese drip off the right edge, stretching slowly, + a drop hanging at the tip
    float drip = 0.13f + 0.05f * sinf(time * 2.0f);
    glm::vec4 cheese = hexColor(0xF7C64F);
    Renderer::draw(Shape::Quad, part(m, 0.205f, -0.1f - drip * 0.5f, 0.065f, drip), cheese);
    Renderer::draw(Shape::Circle, part(m, 0.205f, -0.1f - drip, 0.09f, 0.1f), cheese);
    Renderer::draw(Shape::Circle, part(m, 0.0f, PIZZA_TIP_Y - 0.02f, 0.07f, 0.09f), cheese);
    // shine on the cheese
    Renderer::draw(Shape::Quad, part(m, -0.16f, 0.02f, 0.035f, 0.32f, -20.0f), withAlpha(WHITE, 0.3f));
}

// Raphael's sai (req 75): pointed blade, curved side prongs (yoku), guard knob, wrapped grip, pommel.
// Points right at spin 0, about 36 units long. Built from quads/triangles/circles like the characters.
void PropModels::drawSai(const glm::mat4& base, float spinDegrees)
{
    const glm::vec4 steel = hexColor(0xC8D0D8);
    const glm::vec4 steelDark = hexColor(0x8E98A3);
    const glm::vec4 leather = hexColor(0x5A2E1A);
    const glm::vec4 wrap = hexColor(0x8B1E1E);

    glm::mat4 m = rotate2D(base, -spinDegrees);

    // blade (monouchi): long bar with a darker ridge and a triangle tip
    Renderer::draw(Shape::Quad, part(m, 8.0f, 0.0f, 20.0f, 3.6f), steel);
    Renderer::draw(Shape::Quad, part(m, 8.0f, -0.6f, 20.0f, 1.0f), steelDark);
    Renderer::draw(Shape::Triangle, part(m, 20.5f, 0.0f, 3.6f, 5.0f, -90.0f), steel);

    // two prongs: go out from the guard, then bend forward along the blade
    for (int side = -1; side <= 1; side += 2) {
        glm::mat4 prong = joint(m, -1.0f, 1.5f * side, 55.0f * side);
        Renderer::draw(Shape::Quad, part(prong, 3.5f, 0.0f, 7.0f, 2.4f), steel);
        glm::mat4 tip = joint(prong, 7.0f, 0.0f, -55.0f * side);
        Renderer::draw(Shape::Quad, part(tip, 3.0f, 0.0f, 6.0f, 2.2f), steel);
        Renderer::draw(Shape::Triangle, part(tip, 7.0f, 0.0f, 2.2f, 3.0f, -90.0f), steel);
    }
    Renderer::draw(Shape::Circle, part(m, -1.0f, 0.0f, 5.5f, 5.5f), steelDark);   // guard knob

    // grip with wrap bands + pommel
    Renderer::draw(Shape::Quad, part(m, -9.0f, 0.0f, 12.0f, 4.6f), leather);
    for (int i = 0; i < 3; ++i)
        Renderer::draw(Shape::Quad, part(m, -13.0f + 4.0f * i, 0.0f, 1.6f, 4.8f, 20.0f), wrap);
    Renderer::draw(Shape::Circle, part(m, -15.5f, 0.0f, 5.0f, 5.0f), steelDark);
}

// Foot Soldier shuriken (req 92): 4-point throwing star, clearly not a sai. Four triangle points
// with a light bevel on one side, a round hub and a hole. About 26 units across.
void PropModels::drawShuriken(const glm::mat4& base, float spinDegrees)
{
    const glm::vec4 steel = hexColor(0x9AA0A8);
    const glm::vec4 shine = hexColor(0xD5DAE0);
    const glm::vec4 dark = hexColor(0x30343A);

    glm::mat4 m = rotate2D(base, spinDegrees);
    for (int i = 0; i < 4; ++i) {
        glm::mat4 point = rotate2D(m, 90.0f * i);
        Renderer::draw(Shape::Triangle, part(point, 0.0f, 7.5f, 9.0f, 13.0f), steel);
        Renderer::draw(Shape::RightTriangle, part(point, 1.2f, 6.5f, 2.4f, 10.0f), shine);   // bevel
    }
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 10.0f, 10.0f), steel);
    Renderer::draw(Shape::Ring, part(m, 0.0f, 0.0f, 10.0f, 10.0f), dark);
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 3.5f, 3.5f), dark);
}

// Bebop's blaster bolt (req 92): glowing elongated shot that flies left, with a fading tail on
// its right, a pulsing glow and a white-hot core. Nothing like the sai or the shuriken.
void PropModels::drawBlasterBolt(const glm::mat4& base, float time)
{
    const glm::vec4 glow = hexColor(0xFF5A1F);
    const glm::vec4 hot = hexColor(0xFFD27A);

    float pulse = 0.8f + 0.2f * sinf(time * 20.0f);
    // tail: two stretched triangles pointing back (right), fading out
    Renderer::draw(Shape::Triangle, part(base, 16.0f, 0.0f, 12.0f * pulse, 26.0f, -90.0f), withAlpha(glow, 0.35f));
    Renderer::draw(Shape::Triangle, part(base, 12.0f, 0.0f, 6.0f, 18.0f, -90.0f), withAlpha(hot, 0.5f));
    // glow + body + core
    Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, 36.0f * pulse, 16.0f * pulse), withAlpha(glow, 0.45f));
    Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, 26.0f, 9.0f), glow);
    Renderer::draw(Shape::Circle, part(base, -3.0f, 0.0f, 18.0f, 5.0f), hot);
    Renderer::draw(Shape::Circle, part(base, -8.0f, 0.0f, 5.0f, 3.0f), hexColor(0xFFFFFF));
}
