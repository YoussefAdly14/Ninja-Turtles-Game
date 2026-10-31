#include "scene/Environment.h"
#include "core/Renderer.h"
#include "core/Mesh.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "game/LaneManager.h"
#include "Config.h"
#include <cmath>
#include <cstdlib>
#include <vector>

// The sewer around the fight (req 111-118). Everything is triangles: the static parts (brick wall,
// vault arches, pillars, walkways, water) are built ONCE in init() as per-vertex-coloured meshes
// (Lab 3 "several shapes in one VBO" + EBO, Lab 2 colour interpolation for the gradients), and the
// animated props are building-block shapes moved by uniforms only (Lab 2/4 transforms + time).
// Everything stays darker and less saturated than the characters so they always read first.

// ---------------- layout (world units) ----------------
const float BAY_WIDTH = 360.0f;                 // pillar to pillar
const float FIRST_PILLAR_X = 260.0f;            // first pillar right of the Lair
const float PILLAR_WIDTH = 44.0f;
const float ARCH_SPRING_Y = 632.0f;             // where the vault starts to curve (just above the top walkway)
const float ARCH_OUTER = BAY_WIDTH * 0.5f;      // neighbouring arches meet on the pillar
const float ARCH_INNER = ARCH_OUTER - 28.0f;
const float WATER_TOP = 44.0f;                  // water surface in the channel under the bottom walkway
const float LEDGE_FACE = 12.0f;                 // walkway front face, above laneBottom()
const float LEDGE_BACK = 32.0f;                 // back edge of the walkway surface (feet are at LANE_FLOOR_OFFSET)
const float BRICK_W = 44.0f, BRICK_H = 18.0f, MORTAR = 4.0f;
const float PI_F = 3.14159265f;

// ---------------- colours (dark + desaturated on purpose) ----------------
const glm::vec4 MORTAR_COLOR   = hexColor(0x1C2421);
const glm::vec4 MOSS           = hexColor(0x3E5A36);
const glm::vec4 CEILING_DARK   = hexColor(0x101614);
const glm::vec4 CEILING_LIT    = hexColor(0x222C28);
const glm::vec4 ARCH_BRICK     = hexColor(0x4C5B53);
const glm::vec4 KEYSTONE       = hexColor(0x66726A);
const glm::vec4 STONE          = hexColor(0x4A554E);
const glm::vec4 STONE_DARK     = hexColor(0x2E3833);
const glm::vec4 LEDGE_FACE_TOP = hexColor(0x464A42);
const glm::vec4 LEDGE_FACE_BOT = hexColor(0x2A2D29);
const glm::vec4 LEDGE_LIP      = hexColor(0x62675B);
const glm::vec4 WALK_FRONT     = hexColor(0x555A50);
const glm::vec4 WALK_BACK      = hexColor(0x3B3F39);
const glm::vec4 WATER_SURFACE  = hexColor(0x2D5547);
const glm::vec4 WATER_DEEP     = hexColor(0x0E211B);
const glm::vec4 WATER_SHEEN    = hexColor(0x5E8F7E);
const glm::vec4 PIPE           = hexColor(0x4E5A55);
const glm::vec4 PIPE_LIGHT     = hexColor(0x6F7C76);
const glm::vec4 PIPE_DARK      = hexColor(0x2F3834);
const glm::vec4 RUST           = hexColor(0x7A4E2E);
const glm::vec4 LAMP_WARM      = hexColor(0xFFE2A0);
const glm::vec4 CARDBOARD      = hexColor(0x9C7C52);
const glm::vec4 WOOD           = hexColor(0x6A4E32);

// ---------------- static meshes (init/shutdown) ----------------
static Mesh g_wall;        // mortar + ~1300 bricks with the lighting gradient baked in
static Mesh g_spandrel;    // dark ceiling between two arches (one bay, drawn per bay)
static Mesh g_arch;        // brick voussoir ring + keystone (one bay)
static Mesh g_pillar;      // stone pillar: plinth, shaft blocks, capital
static Mesh g_ledges;      // the 4 walkways: front face, lip, surface, slab joints
static Mesh g_water;       // channel back wall + water body
static Mesh g_waterPatch;  // same gradient as the water's lower part: tints things that are "under water"
static Mesh g_thinRing;    // ripple rings (outer radius 0.5)
static Mesh g_sector;      // 60 degree pie slice for the hazard trefoil
static Mesh g_hexagon;     // turtle-shell graffiti

// Lab 3 "several shapes in one VBO": every brick/stone is 4 vertices + 6 indices (EBO).
struct MeshBuilder
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    void vertex(glm::vec2 p, glm::vec3 c) { vertices.insert(vertices.end(), { p.x, p.y, 0.0f, c.r, c.g, c.b }); }

    // any 4-sided shape: bottom-left, bottom-right, top-right, top-left, one colour per corner
    void quad(glm::vec2 bl, glm::vec2 br, glm::vec2 tr, glm::vec2 tl,
              glm::vec3 cbl, glm::vec3 cbr, glm::vec3 ctr, glm::vec3 ctl)
    {
        unsigned int i = static_cast<unsigned int>(vertices.size() / 6);
        vertex(bl, cbl); vertex(br, cbr); vertex(tr, ctr); vertex(tl, ctl);
        indices.insert(indices.end(), { i, i + 1, i + 2,   i + 2, i + 3, i });
    }
    // axis-aligned rectangle with a bottom -> top gradient
    void rect(float x0, float y0, float x1, float y1, glm::vec3 bottom, glm::vec3 top)
    {
        quad({ x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 }, bottom, bottom, top, top);
    }
    Mesh build() const
    {
        return createIndexedMesh(vertices.data(), vertices.size() * sizeof(float),
                                 indices.data(), indices.size() * sizeof(unsigned int));
    }
};

// cheap repeatable "random" 0..1 so the wall looks the same every run (doesn't touch rand())
static float hash01(int a, int b)
{
    unsigned int h = static_cast<unsigned int>(a) * 374761393u + static_cast<unsigned int>(b) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return static_cast<float>((h ^ (h >> 16)) % 1000u) / 1000.0f;
}

static glm::vec3 rgb(const glm::vec4& c) { return glm::vec3(c); }
static glm::vec4 shade(const glm::vec4& c, float k) { return glm::vec4(glm::vec3(c) * k, c.a); }   // darker/lighter, same alpha

// How much light reaches the back wall at (x, y): darker in the shadow under each walkway, a bit
// grimy at the foot of the wall, dark up in the vault, and fading towards the far (enemy) end.
static float wallLight(float x, float y)
{
    float light;
    if (y >= PLAYFIELD_TOP) {
        light = 0.62f - 0.3f * (y - PLAYFIELD_TOP) / HUD_HEIGHT;
    }
    else {
        int lane = static_cast<int>((PLAYFIELD_TOP - y) / LANE_HEIGHT);
        if (lane > LANE_COUNT - 1) lane = LANE_COUNT - 1;
        float t = (y - laneBottom(lane)) / LANE_HEIGHT;     // 0 at the walkway, 1 under the next one
        light = 1.0f - 0.42f * t * t - 0.15f * powf(1.0f - t, 4.0f);
        if (lane == 0) light -= 0.12f * t;                    // the vault above is darker still
    }
    return light * (1.0f - 0.2f * (x / WORLD_WIDTH));
}

static Mesh buildWall()
{
    MeshBuilder b;
    const float y0 = PLAYFIELD_BOTTOM, y1 = WORLD_HEIGHT;
    int rows = static_cast<int>((y1 - y0) / (BRICK_H + MORTAR)) + 1;

    for (int row = 0; row < rows; ++row) {
        float rowY = y0 + row * (BRICK_H + MORTAR);
        // mortar strip for this row, in chunks so it follows the light too
        for (float x = 0.0f; x < WORLD_WIDTH; x += 200.0f) {
            float top = rowY + BRICK_H + MORTAR;
            b.quad({ x, rowY }, { x + 200.0f, rowY }, { x + 200.0f, top }, { x, top },
                   rgb(MORTAR_COLOR) * wallLight(x, rowY), rgb(MORTAR_COLOR) * wallLight(x + 200.0f, rowY),
                   rgb(MORTAR_COLOR) * wallLight(x + 200.0f, top), rgb(MORTAR_COLOR) * wallLight(x, top));
        }
        // running bond: every other row shifted by half a brick
        float offset = (row % 2) ? -(BRICK_W + MORTAR) * 0.5f : 0.0f;
        for (int col = 0; ; ++col) {
            float bx0 = offset + col * (BRICK_W + MORTAR) + MORTAR * 0.5f;
            if (bx0 > WORLD_WIDTH) break;
            float bx1 = bx0 + BRICK_W;
            float by0 = rowY + MORTAR * 0.5f, by1 = by0 + BRICK_H;

            glm::vec3 c = rgb(SEWER_WALL) * (0.74f + 0.28f * hash01(row, col));
            float stain = hash01(col + 77, row * 3 + 5);
            if (stain < 0.10f)      c = glm::mix(c, rgb(MOSS), 0.5f);    // moss
            else if (stain > 0.93f) c *= 0.7f;                           // wet, darker brick
            glm::vec3 lit = c * 1.08f;                                   // top edge catches a bit of light

            b.quad({ bx0, by0 }, { bx1, by0 }, { bx1, by1 }, { bx0, by1 },
                   c * wallLight(bx0, by0), c * wallLight(bx1, by0),
                   lit * wallLight(bx1, by1), lit * wallLight(bx0, by1));
        }
    }
    return b.build();
}

// Region between the outer edge of one arch and the ceiling: strips from each arc point straight up.
// Local coordinates: arch centre on the springing line at (0, 0).
static Mesh buildSpandrel()
{
    MeshBuilder b;
    const int segments = 24;
    const float top = WORLD_HEIGHT - ARCH_SPRING_Y;
    for (int i = 0; i < segments; ++i) {
        float a0 = PI_F * i / segments, a1 = PI_F * (i + 1) / segments;
        glm::vec2 p0(ARCH_OUTER * cosf(a0), ARCH_OUTER * sinf(a0));
        glm::vec2 p1(ARCH_OUTER * cosf(a1), ARCH_OUTER * sinf(a1));
        // a1 is further left, so p1 is the "bottom-left" corner
        b.quad(p1, p0, { p0.x, top }, { p1.x, top },
               rgb(CEILING_LIT), rgb(CEILING_LIT), rgb(CEILING_DARK), rgb(CEILING_DARK));
    }
    return b.build();
}

static Mesh buildArch()
{
    MeshBuilder b;
    const int segments = 32;
    // mortar band behind the voussoirs
    for (int i = 0; i < segments; ++i) {
        float a0 = PI_F * i / segments, a1 = PI_F * (i + 1) / segments;
        float ri = ARCH_INNER - 2.0f, ro = ARCH_OUTER + 1.0f;
        b.quad({ ri * cosf(a1), ri * sinf(a1) }, { ri * cosf(a0), ri * sinf(a0) },
               { ro * cosf(a0), ro * sinf(a0) }, { ro * cosf(a1), ro * sinf(a1) },
               rgb(MORTAR_COLOR), rgb(MORTAR_COLOR), rgb(MORTAR_COLOR) * 0.6f, rgb(MORTAR_COLOR) * 0.6f);
    }
    // 15 wedge bricks (voussoirs), the middle one is the taller keystone
    const int count = 15;
    const float gap = 3.0f / ((ARCH_INNER + ARCH_OUTER) * 0.5f);   // ~3 units of mortar, as an angle
    for (int k = 0; k < count; ++k) {
        float a0 = PI_F * k / count + gap * 0.5f;
        float a1 = PI_F * (k + 1) / count - gap * 0.5f;
        bool key = (k == count / 2);
        float ri = ARCH_INNER, ro = ARCH_OUTER + (key ? 12.0f : 0.0f);
        glm::vec3 c = key ? rgb(KEYSTONE) : rgb(ARCH_BRICK) * (0.85f + 0.25f * hash01(k, 9));
        // light comes from below (the lamps), so the inner edge is brighter
        const int slices = 3;
        for (int s = 0; s < slices; ++s) {
            float s0 = a0 + (a1 - a0) * s / slices, s1 = a0 + (a1 - a0) * (s + 1) / slices;
            b.quad({ ri * cosf(s1), ri * sinf(s1) }, { ri * cosf(s0), ri * sinf(s0) },
                   { ro * cosf(s0), ro * sinf(s0) }, { ro * cosf(s1), ro * sinf(s1) },
                   c * 1.1f, c * 1.1f, c * 0.6f, c * 0.6f);
        }
    }
    return b.build();
}

// Local coordinates: x centred on the pillar, y in world units (it always stands in the same place).
static Mesh buildPillar()
{
    MeshBuilder b;
    const float w = PILLAR_WIDTH * 0.5f;
    const float shaftTop = ARCH_SPRING_Y - 22.0f;
    glm::vec3 light = rgb(STONE) * 1.15f, dark = rgb(STONE_DARK);

    b.rect(-w, WATER_TOP, w, shaftTop, rgb(MORTAR_COLOR), rgb(MORTAR_COLOR));        // joints behind the blocks
    // shaft blocks: lit on the left, shaded on the right (square pillar turning away from the lamps)
    int row = 0;
    for (float y = WATER_TOP + 24.0f; y < shaftTop - 4.0f; y += 30.0f, ++row) {
        float y1 = fminf(y + 27.0f, shaftTop);
        float shade = 0.85f + 0.25f * hash01(row, 3);
        glm::vec3 l = light * shade, d = dark * shade;
        if (y < 140.0f) { l = glm::mix(l, rgb(MOSS), 0.35f); d = glm::mix(d, rgb(MOSS) * 0.6f, 0.35f); }   // damp moss near the water
        b.quad({ -w + 1.5f, y }, { w - 1.5f, y }, { w - 1.5f, y1 }, { -w + 1.5f, y1 }, l, d, d, l);
        b.rect(-w + 1.5f, y, -w + 6.0f, y1, l * 1.15f, l * 1.15f);                    // edge highlight
    }
    // plinth (base) and capital (top), both wider than the shaft
    b.quad({ -w - 6.0f, WATER_TOP }, { w + 6.0f, WATER_TOP }, { w + 6.0f, WATER_TOP + 24.0f }, { -w - 6.0f, WATER_TOP + 24.0f },
           glm::mix(light, rgb(MOSS), 0.4f), glm::mix(dark, rgb(MOSS), 0.4f), dark, light);
    b.quad({ -w - 4.0f, shaftTop }, { w + 4.0f, shaftTop }, { w + 9.0f, shaftTop + 14.0f }, { -w - 9.0f, shaftTop + 14.0f },
           dark, dark, light, light);
    b.rect(-w - 12.0f, shaftTop + 14.0f, w + 12.0f, ARCH_SPRING_Y, light * 0.9f, light * 1.1f);   // abacus slab
    return b.build();
}

static Mesh buildLedges()
{
    MeshBuilder b;
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        float y0 = laneBottom(lane);
        float yFace = y0 + LEDGE_FACE, yBack = y0 + LEDGE_BACK;

        // front face (concrete curb) with block joints
        b.rect(0.0f, y0, WORLD_WIDTH, yFace, rgb(LEDGE_FACE_BOT), rgb(LEDGE_FACE_TOP));
        for (float x = 40.0f + lane * 37.0f; x < WORLD_WIDTH; x += 120.0f)
            b.rect(x, y0, x + 3.0f, yFace, rgb(LEDGE_FACE_BOT) * 0.6f, rgb(LEDGE_FACE_BOT) * 0.6f);
        b.rect(0.0f, y0, WORLD_WIDTH, y0 + 2.0f, rgb(MORTAR_COLOR) * 0.5f, rgb(MORTAR_COLOR) * 0.5f);   // underside shadow

        // walkway surface seen slightly from above: bright front edge, darker back where it meets the wall
        b.rect(0.0f, yFace, WORLD_WIDTH, yBack, rgb(WALK_FRONT), rgb(WALK_BACK));
        b.rect(0.0f, yFace - 1.0f, WORLD_WIDTH, yFace + 1.5f, rgb(LEDGE_LIP), rgb(LEDGE_LIP));
        b.rect(0.0f, yBack - 2.0f, WORLD_WIDTH, yBack, rgb(WALK_BACK) * 0.6f, rgb(WALK_BACK) * 0.6f);

        // slab joints, slanted for a bit of perspective, + damp patches
        for (float x = 40.0f + lane * 37.0f; x < WORLD_WIDTH; x += 120.0f)
            b.quad({ x, yFace }, { x + 2.5f, yFace }, { x + 10.5f, yBack }, { x + 8.0f, yBack },
                   rgb(WALK_FRONT) * 0.6f, rgb(WALK_FRONT) * 0.6f, rgb(WALK_BACK) * 0.6f, rgb(WALK_BACK) * 0.6f);
        for (int k = 0; k < 6; ++k) {
            float x = 120.0f + 250.0f * k + 90.0f * hash01(lane, k);
            float w = 30.0f + 50.0f * hash01(k, lane + 11);
            glm::vec3 damp = rgb(WALK_FRONT) * 0.82f;
            b.quad({ x, yFace + 3.0f }, { x + w, yFace + 3.0f }, { x + w + 8.0f, yBack - 4.0f }, { x + 6.0f, yBack - 4.0f },
                   damp, damp, rgb(WALK_BACK) * 0.85f, rgb(WALK_BACK) * 0.85f);
        }
    }
    return b.build();
}

static Mesh buildWater()
{
    MeshBuilder b;
    // channel back wall under the bottom walkway, then the water body with a lighter surface
    b.rect(0.0f, WATER_TOP - 2.0f, WORLD_WIDTH, PLAYFIELD_BOTTOM, rgb(MORTAR_COLOR) * 0.7f, rgb(MORTAR_COLOR) * 0.4f);
    b.rect(0.0f, 0.0f, WORLD_WIDTH, WATER_TOP, rgb(WATER_DEEP), rgb(WATER_SURFACE));
    b.rect(0.0f, WATER_TOP - 3.0f, WORLD_WIDTH, WATER_TOP, rgb(WATER_SURFACE), rgb(WATER_SHEEN));
    return b.build();
}

// 1 unit wide (scale x when drawing), from the channel floor up to just under the surface
static Mesh buildWaterPatch()
{
    MeshBuilder b;
    const float top = WATER_TOP - 8.0f;
    b.rect(-0.5f, 0.0f, 0.5f, top, rgb(WATER_DEEP), glm::mix(rgb(WATER_DEEP), rgb(WATER_SURFACE), top / WATER_TOP));
    return b.build();
}

static Mesh buildThinRing()
{
    MeshBuilder b;
    const int segments = 32;
    for (int i = 0; i < segments; ++i) {
        float a0 = 2.0f * PI_F * i / segments, a1 = 2.0f * PI_F * (i + 1) / segments;
        b.quad({ 0.42f * cosf(a0), 0.42f * sinf(a0) }, { 0.42f * cosf(a1), 0.42f * sinf(a1) },
               { 0.5f * cosf(a1), 0.5f * sinf(a1) }, { 0.5f * cosf(a0), 0.5f * sinf(a0) },
               glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f));
    }
    return b.build();
}

void Environment::init()
{
    g_wall = buildWall();
    g_spandrel = buildSpandrel();
    g_arch = buildArch();
    g_pillar = buildPillar();
    g_ledges = buildLedges();
    g_water = buildWater();
    g_waterPatch = buildWaterPatch();
    g_thinRing = buildThinRing();

    // trefoil blade: a 60 degree slice of a circle (fan from the centre, Lab 3 circle idea)
    std::vector<glm::vec2> slice = { { 0.0f, 0.0f } };
    for (int i = 0; i <= 8; ++i) {
        float a = glm::radians(60.0f + 60.0f * i / 8.0f);
        slice.push_back({ 0.5f * cosf(a), 0.5f * sinf(a) });
    }
    g_sector = createPolygonMesh(slice);

    std::vector<glm::vec2> hex;
    for (int i = 0; i < 6; ++i) {
        float a = glm::radians(60.0f * i);
        hex.push_back({ 0.5f * cosf(a), 0.5f * sinf(a) });
    }
    g_hexagon = createPolygonMesh(hex);
}

void Environment::shutdown()
{
    destroyMesh(g_wall);
    destroyMesh(g_spandrel);
    destroyMesh(g_arch);
    destroyMesh(g_pillar);
    destroyMesh(g_ledges);
    destroyMesh(g_water);
    destroyMesh(g_waterPatch);
    destroyMesh(g_thinRing);
    destroyMesh(g_sector);
    destroyMesh(g_hexagon);
}

static float walkwayY(int lane) { return laneBottom(lane) + LANE_FLOOR_OFFSET; }   // middle of the walkway surface

// ===================================================================================
// Element 7 (req 118): the sewer-water stream. Drawn with the background so the menu and the
// cutscene get the animated water too. Current streaks at three depths, wavelets on the surface,
// expanding ripple rings, and floating junk drifting downstream.
// ===================================================================================
static void drawWaterStream(float time)
{
    const glm::mat4 I(1.0f);

    // floating junk drifting downstream (left -> right), bobbing: a can and a plank
    float canX = fmodf(time * 26.0f + 300.0f, WORLD_WIDTH + 200.0f) - 100.0f;
    glm::mat4 can = rotate2D(translate2D(I, canX, 36.0f + 2.0f * sinf(time * 2.3f)), 70.0f + 8.0f * sinf(time * 1.4f));
    Renderer::draw(Shape::Quad, part(can, 0.0f, 0.0f, 10.0f, 18.0f), hexColor(0x6E3A36));
    Renderer::draw(Shape::Quad, part(can, 0.0f, 1.0f, 10.0f, 5.0f), hexColor(0x9A9A90));
    Renderer::draw(Shape::Quad, part(can, 0.0f, 8.5f, 8.0f, 2.0f), hexColor(0x7C7C74));
    float plankX = fmodf(time * 20.0f + 1100.0f, WORLD_WIDTH + 300.0f) - 150.0f;
    glm::mat4 plank = rotate2D(translate2D(I, plankX, 37.0f + 2.0f * sinf(time * 1.8f + 1.0f)), 3.0f * sinf(time * 1.1f));
    Renderer::draw(Shape::Quad, part(plank, 0.0f, 0.0f, 64.0f, 7.0f), shade(WOOD, 0.8f));
    Renderer::draw(Shape::Quad, part(plank, -20.0f, 0.0f, 2.0f, 7.0f), shade(WOOD, 0.5f));
    Renderer::draw(Shape::Quad, part(plank, 18.0f, 0.0f, 2.0f, 7.0f), shade(WOOD, 0.5f));
    // water laps over the bottom of the junk (same colours as the water, so only the junk gets tinted)
    Renderer::draw(g_waterPatch, scale2D(I, WORLD_WIDTH * 2.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.6f));

    // current streaks: deeper = slower and darker
    for (int depth = 0; depth < 3; ++depth) {
        float y = 10.0f + depth * 11.0f;
        float speed = 18.0f + depth * 16.0f;
        float spacing = 130.0f + depth * 30.0f;
        float scroll = fmodf(time * speed, spacing);
        for (float x = -spacing; x < WORLD_WIDTH + spacing; x += spacing) {
            float wobble = 2.0f * sinf(time * 1.7f + x * 0.03f);
            Renderer::drawRect({ x + scroll + depth * 40.0f, y + wobble }, { 40.0f + depth * 14.0f, 2.0f },
                               withAlpha(WATER_SHEEN, 0.18f + depth * 0.08f));
        }
    }
    // wavelets on the surface line
    float scroll = fmodf(time * 30.0f, 64.0f);
    for (float x = -64.0f; x < WORLD_WIDTH + 64.0f; x += 64.0f) {
        float lift = 1.5f * sinf(time * 3.0f + x * 0.05f);
        Renderer::draw(Shape::Semicircle, part(I, x + scroll, WATER_TOP - 3.0f + lift, 34.0f, 5.0f), withAlpha(WATER_SHEEN, 0.45f));
    }
    // ripple rings where drips land: grow and fade, each on its own timer
    const float ripples[][2] = { { 330.0f, 0.0f }, { 720.0f, 0.6f }, { 1085.0f, 1.3f }, { 1450.0f, 0.3f }, { 1290.0f, 2.1f } };
    for (const auto& r : ripples) {
        float phase = fmodf(time * 0.55f + r[1], 1.0f);
        float size = 8.0f + 46.0f * phase;
        Renderer::draw(g_thinRing, part(I, r[0], 22.0f + 6.0f * (r[1] - 1.0f), size, size * 0.22f), withAlpha(WATER_SHEEN, 0.5f * (1.0f - phase)));
    }
}

// Back wall, vaulted brick ceiling, pillars, the four walkways and the water channel (req 111, 39).
void Environment::drawBackground(float time)
{
    const glm::mat4 I(1.0f);

    Renderer::draw(g_wall, I, WHITE);

    // the vault: one arch per bay, the ceiling above it in shadow (semi-transparent so bricks show through)
    for (float cx = FIRST_PILLAR_X - BAY_WIDTH * 0.5f; cx < WORLD_WIDTH + BAY_WIDTH; cx += BAY_WIDTH) {
        glm::mat4 m = translate2D(I, cx, ARCH_SPRING_Y);
        Renderer::draw(g_spandrel, m, glm::vec4(1.0f, 1.0f, 1.0f, 0.88f));
        Renderer::draw(g_arch, m, WHITE);
    }
    for (float px = FIRST_PILLAR_X - BAY_WIDTH; px < WORLD_WIDTH + BAY_WIDTH; px += BAY_WIDTH)
        Renderer::draw(g_pillar, translate2D(I, px, 0.0f), WHITE);

    Renderer::draw(g_water, I, WHITE);
    drawWaterStream(time);
    Renderer::draw(g_ledges, I, WHITE);
}

// ===================================================================================
// Element 1 (req 112): sewer pipes. A pair of big ceiling mains (seen in the menu/cutscene, under
// the HUD in game) and vertical drop pipes beside the pillars, broken at every walkway so they
// run BEHIND it. Flanges with bolts, rust bands, a valve wheel, a pressure gauge, an outfall into
// the channel, and drips falling from the flanges.
// ===================================================================================
static void drawPipeRun(float x, float y0, float y1, float width)
{
    float cx = x, cy = (y0 + y1) * 0.5f, h = y1 - y0;
    Renderer::drawRect({ cx, cy }, { width, h }, PIPE);
    Renderer::drawRect({ cx - width * 0.28f, cy }, { width * 0.18f, h }, PIPE_LIGHT);   // round highlight
    Renderer::drawRect({ cx + width * 0.34f, cy }, { width * 0.22f, h }, PIPE_DARK);    // shadow side
}

static void drawFlange(float x, float y, float width)
{
    Renderer::drawRect({ x, y }, { width + 10.0f, 8.0f }, PIPE_DARK);
    Renderer::drawRect({ x, y + 1.5f }, { width + 10.0f, 3.0f }, shade(PIPE_LIGHT, 0.9f));
    Renderer::drawCircle({ x - width * 0.5f - 2.0f, y }, 2.0f, hexColor(0x9AA39C));
    Renderer::drawCircle({ x + width * 0.5f + 2.0f, y }, 2.0f, hexColor(0x9AA39C));
}

// a drop falls from (x, yStart) to yEnd, then splashes; phase 0..1
static void drawDrip(float x, float yStart, float yEnd, float phase)
{
    const float fallPart = 0.8f;
    if (phase < fallPart) {
        float t = phase / fallPart;
        float y = yStart - (yStart - yEnd) * t * t;     // accelerating fall
        float stretch = 1.0f + 1.5f * t;
        Renderer::draw(Shape::Circle, part(glm::mat4(1.0f), x, y, 4.0f, 4.0f * stretch), withAlpha(hexColor(0xA9C9BE), 0.7f));
    }
    else {
        float t = (phase - fallPart) / (1.0f - fallPart);
        Renderer::draw(g_thinRing, part(glm::mat4(1.0f), x, yEnd, 6.0f + 16.0f * t, 2.5f + 3.0f * t), withAlpha(hexColor(0xA9C9BE), 0.6f * (1.0f - t)));
    }
}

static void drawValveWheel(float x, float y, float time)
{
    glm::mat4 m = rotate2D(translate2D(glm::mat4(1.0f), x, y), 10.0f * sinf(time * 0.4f));
    Renderer::draw(Shape::Quad, part(m, -10.0f, 0.0f, 16.0f, 6.0f), PIPE_DARK);           // stem into the pipe
    for (int i = 0; i < 4; ++i)
        Renderer::draw(Shape::Quad, part(rotate2D(m, 45.0f * i), 0.0f, 0.0f, 26.0f, 2.5f), hexColor(0x8A3A30));
    Renderer::draw(Shape::Ring, part(m, 0.0f, 0.0f, 30.0f, 30.0f), hexColor(0x9E4234));
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 8.0f, 8.0f), PIPE_LIGHT);
}

static void drawGauge(float x, float y, float time)
{
    glm::mat4 m = translate2D(glm::mat4(1.0f), x, y);
    Renderer::draw(Shape::Quad, part(m, -12.0f, 0.0f, 10.0f, 5.0f), PIPE_DARK);
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 20.0f, 20.0f), PIPE_DARK);
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 15.0f, 15.0f), hexColor(0xB9B6A2));
    Renderer::draw(Shape::Quad, part(m, 3.0f, 3.0f, 3.0f, 3.0f, 45.0f), hexColor(0xA03A30));   // red zone mark
    float needle = -40.0f + 25.0f * sinf(time * 1.3f) + 6.0f * sinf(time * 9.0f);             // nervous needle
    Renderer::draw(Shape::Quad, part(joint(m, 0.0f, 0.0f, needle), 0.0f, 3.0f, 1.5f, 7.0f), hexColor(0x202020));
}

static void drawPipes(float time)
{
    // ceiling mains (top of the vault), with hanging brackets
    const float mainY[2] = { 820.0f, 846.0f };
    const float mainW[2] = { 24.0f, 16.0f };
    for (int k = 0; k < 2; ++k) {
        float y = mainY[k], w = mainW[k];
        Renderer::drawRect({ WORLD_WIDTH * 0.5f, y }, { WORLD_WIDTH, w }, PIPE);
        Renderer::drawRect({ WORLD_WIDTH * 0.5f, y + w * 0.25f }, { WORLD_WIDTH, w * 0.18f }, PIPE_LIGHT);
        Renderer::drawRect({ WORLD_WIDTH * 0.5f, y - w * 0.32f }, { WORLD_WIDTH, w * 0.22f }, PIPE_DARK);
        for (float x = 230.0f + k * 90.0f; x < WORLD_WIDTH; x += 180.0f) {
            Renderer::drawRect({ x, y }, { 8.0f, w + 10.0f }, PIPE_DARK);
            Renderer::drawRect({ x - 1.5f, y }, { 3.0f, w + 10.0f }, shade(PIPE_LIGHT, 0.9f));
            Renderer::drawRect({ x + 22.0f, y }, { 12.0f, w + 1.0f }, withAlpha(RUST, 0.7f));   // rust band
        }
    }
    for (float x = 320.0f; x < WORLD_WIDTH; x += 360.0f)
        Renderer::drawRect({ x, 870.0f }, { 4.0f, 48.0f }, PIPE_DARK);   // hanger rods

    // vertical drop pipes beside two pillars + a thin conduit, broken at each walkway
    struct Drop { float x, width; };
    const Drop drops[] = { { FIRST_PILLAR_X + BAY_WIDTH + 36.0f, 22.0f },
                           { FIRST_PILLAR_X + 3.0f * BAY_WIDTH - 36.0f, 22.0f },
                           { FIRST_PILLAR_X + 2.0f * BAY_WIDTH + 32.0f, 10.0f } };
    for (int d = 0; d < 3; ++d) {
        const Drop& p = drops[d];
        // segment above the top walkway goes up into the ceiling mains
        float segTop[LANE_COUNT + 1], segBottom[LANE_COUNT + 1];
        segTop[0] = mainY[0];
        segBottom[0] = laneBottom(0) + LEDGE_BACK;
        for (int lane = 1; lane < LANE_COUNT; ++lane) { segTop[lane] = laneTop(lane); segBottom[lane] = laneBottom(lane) + LEDGE_BACK; }
        segTop[LANE_COUNT] = PLAYFIELD_BOTTOM;
        segBottom[LANE_COUNT] = WATER_TOP + 4.0f;
        for (int s = 0; s <= LANE_COUNT; ++s) {
            drawPipeRun(p.x, segBottom[s], segTop[s], p.width);
            if (p.width > 12.0f && s < LANE_COUNT) {
                float flangeY = segTop[s] - 26.0f;
                drawFlange(p.x, flangeY, p.width);
                drawFlange(p.x, segBottom[s] + 10.0f, p.width);
                Renderer::drawRect({ p.x, (segTop[s] + segBottom[s]) * 0.5f - 18.0f }, { p.width + 1.0f, 9.0f }, withAlpha(RUST, 0.75f));
                // leaky flange: a drop every ~2 s, falling to this walkway
                float phase = fmodf(time * 0.45f + d * 0.37f + s * 0.21f, 1.0f);
                drawDrip(p.x + p.width * 0.5f + 3.0f, flangeY - 4.0f, segBottom[s] - 8.0f, phase);
            }
            else if (s < LANE_COUNT) {
                // conduit clamps
                Renderer::drawRect({ p.x, segTop[s] - 30.0f }, { p.width + 8.0f, 5.0f }, PIPE_DARK);
                Renderer::drawRect({ p.x, segBottom[s] + 40.0f }, { p.width + 8.0f, 5.0f }, PIPE_DARK);
            }
        }
    }
    drawValveWheel(drops[0].x - 22.0f, laneCenterY(2) + 30.0f, time);
    drawGauge(drops[1].x - 20.0f, laneCenterY(1) + 40.0f, time);

    // outfall: the first drop pipe bends out of the channel wall and pours into the stream
    float ox = drops[0].x;
    Renderer::drawRect({ ox + 14.0f, 52.0f }, { 28.0f, 14.0f }, PIPE);
    Renderer::drawRect({ ox + 14.0f, 56.0f }, { 28.0f, 3.0f }, PIPE_LIGHT);
    Renderer::drawRect({ ox + 30.0f, 52.0f }, { 6.0f, 18.0f }, PIPE_DARK);
    float flow = fmodf(time * 3.0f, 1.0f);
    Renderer::drawRect({ ox + 38.0f, 44.0f }, { 9.0f, 14.0f }, withAlpha(hexColor(0x7FA89A), 0.55f));
    Renderer::drawRect({ ox + 37.0f, 48.0f - 8.0f * flow }, { 3.0f, 4.0f }, withAlpha(WHITE, 0.35f));
    for (int i = 0; i < 3; ++i) {   // foam where it hits the water
        float t = fmodf(time * 1.5f + i / 3.0f, 1.0f);
        Renderer::drawCircle({ ox + 34.0f + 14.0f * t + 4.0f * i, WATER_TOP - 2.0f }, 4.0f * (1.0f - t) + 1.5f, withAlpha(hexColor(0xC9DDD5), 0.5f * (1.0f - t)));
    }
}

// ===================================================================================
// Element 2 (req 113): mutagen barrels, the reason Shredder is here. Rusty drums with ribs, a lid,
// a hazard trefoil label, green ooze spilling over the lid and a pulsing glow + puddle.
// Standing against the back wall on the walkways (behind the characters) and one floating in the channel.
// ===================================================================================
static void drawBarrel(const glm::mat4& base, float time, float phase, bool puddle)
{
    const glm::vec4 drum = hexColor(0x5C5A30), drumDark = hexColor(0x3A3A22), drumLight = hexColor(0x7A7744);
    const float w = 40.0f, h = 54.0f;
    float pulse = 0.5f + 0.5f * sinf(time * 2.4f + phase);

    // glow behind the barrel + puddle on the walkway (flat ellipse)
    Renderer::draw(Shape::Circle, part(base, 0.0f, h * 0.85f, 70.0f, 46.0f), withAlpha(MUTAGEN_GLOW, 0.05f + 0.06f * pulse));
    if (puddle) {
        Renderer::draw(Shape::Circle, part(base, 18.0f, 0.0f, 70.0f, 9.0f), withAlpha(hexColor(0x4FA21A), 0.55f + 0.2f * pulse));
        Renderer::draw(Shape::Circle, part(base, 22.0f, 1.0f, 36.0f, 4.0f), withAlpha(MUTAGEN_GLOW, 0.4f + 0.3f * pulse));
    }
    // drum: body, light + shadow sides, top/bottom rims, two ribs
    Renderer::draw(Shape::Quad, part(base, 0.0f, h * 0.5f, w, h), drum);
    Renderer::draw(Shape::Quad, part(base, -w * 0.3f, h * 0.5f, w * 0.14f, h), drumLight);
    Renderer::draw(Shape::Quad, part(base, w * 0.36f, h * 0.5f, w * 0.18f, h), drumDark);
    Renderer::draw(Shape::Quad, part(base, 0.0f, 2.5f, w + 3.0f, 5.0f), drumDark);
    for (int i = 1; i <= 2; ++i) {
        Renderer::draw(Shape::Quad, part(base, 0.0f, h * i / 3.0f, w + 2.0f, 4.0f), drumDark);
        Renderer::draw(Shape::Quad, part(base, 0.0f, h * i / 3.0f + 1.5f, w + 2.0f, 1.2f), drumLight);
    }
    // rust spots
    Renderer::draw(Shape::Circle, part(base, -11.0f, 8.0f, 7.0f, 5.0f), withAlpha(RUST, 0.8f));
    Renderer::draw(Shape::Circle, part(base, 13.0f, 42.0f, 5.0f, 4.0f), withAlpha(RUST, 0.7f));
    // hazard label: pale disc, black trefoil, centre dot
    glm::mat4 label = translate2D(base, 0.0f, h * 0.5f);
    Renderer::draw(Shape::Circle, part(label, 0.0f, 0.0f, 19.0f, 15.0f), hexColor(0xC9B85A));
    for (int i = 0; i < 3; ++i)
        Renderer::draw(g_sector, scale2D(rotate2D(label, 120.0f * i), 15.0f, 13.0f), hexColor(0x1E1E14));
    Renderer::draw(Shape::Circle, part(label, 0.0f, 0.0f, 5.0f, 5.0f), hexColor(0xC9B85A));
    Renderer::draw(Shape::Circle, part(label, 0.0f, 0.0f, 3.0f, 3.0f), hexColor(0x1E1E14));
    // lid seen from slightly above, ooze welling up and running down the side
    Renderer::draw(Shape::Circle, part(base, 0.0f, h, w + 2.0f, 10.0f), drumDark);
    glm::vec4 ooze = glm::mix(hexColor(0x4FB51A), MUTAGEN_GLOW, pulse);
    Renderer::draw(Shape::Circle, part(base, 0.0f, h + 1.0f, w - 2.0f, 8.0f + 2.0f * pulse), ooze);   // brimming over
    Renderer::draw(Shape::Circle, part(base, -6.0f, h + 4.0f, 12.0f, 6.0f + 2.0f * pulse), ooze);     // bubbling up
    Renderer::draw(Shape::Circle, part(base, 7.0f, h + 3.0f, 9.0f, 5.0f), ooze);
    // three fat drips running down the side, slowly stretching
    const float drips[3][2] = { { -13.0f, 0.0f }, { 1.0f, 1.7f }, { 14.0f, 3.1f } };
    for (const auto& d : drips) {
        float len = 7.0f + 9.0f * (0.5f + 0.5f * sinf(time * 1.3f + phase + d[1]));
        Renderer::draw(Shape::Quad, part(base, d[0], h - len * 0.5f, 6.0f, len), ooze);
        Renderer::draw(Shape::Circle, part(base, d[0], h - len, 8.0f, 8.0f), ooze);
    }
    Renderer::draw(Shape::Circle, part(base, -6.0f, h + 5.5f, 5.0f, 2.0f), withAlpha(WHITE, 0.55f));   // wet shine
}

static void drawMutagenBarrels(float time)
{
    const glm::mat4 I(1.0f);
    float back = LEDGE_BACK - 6.0f;   // stand near the back of the walkway, against the wall

    // a pair at the far end of the top lane (where the Foot Clan comes in)
    drawBarrel(translate2D(I, 1538.0f, laneBottom(0) + back), time, 0.0f, true);
    drawBarrel(scale2D(translate2D(I, 1576.0f, laneBottom(0) + back + 2.0f), 0.8f, 0.8f), time, 1.0f, false);
    // lane 2: one standing, one knocked over and leaking
    drawBarrel(translate2D(I, 1470.0f, laneBottom(2) + back), time, 2.0f, false);
    glm::mat4 tipped = rotate2D(translate2D(I, 1530.0f, laneBottom(2) + back + 16.0f), 90.0f);
    drawBarrel(translate2D(tipped, 0.0f, -27.0f), time, 2.7f, false);
    Renderer::draw(Shape::Circle, part(I, 1468.0f, laneBottom(2) + LANE_FLOOR_OFFSET, 120.0f, 10.0f),
                   withAlpha(hexColor(0x4FA21A), 0.45f + 0.15f * sinf(time * 2.4f)));
    // lane 1, mid-way: a lone barrel
    drawBarrel(scale2D(translate2D(I, 1010.0f, laneBottom(1) + back), 0.9f, 0.9f), time, 4.0f, true);

    // one floating in the channel, half under water, leaving a green trail downstream
    float bob = 2.5f * sinf(time * 1.6f);
    glm::mat4 floating = rotate2D(translate2D(I, 1215.0f, 34.0f + bob), 78.0f + 5.0f * sinf(time * 0.9f));
    drawBarrel(scale2D(translate2D(floating, 0.0f, -27.0f), 0.8f, 0.8f), time, 5.0f, false);
    Renderer::draw(g_waterPatch, scale2D(translate2D(I, 1215.0f, 0.0f), 80.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.75f));
    for (int i = 0; i < 4; ++i) {
        float t = fmodf(time * 0.3f + i * 0.25f, 1.0f);
        Renderer::draw(Shape::Circle, part(I, 1250.0f + 160.0f * t, 26.0f + 3.0f * sinf(time + i), 40.0f, 5.0f),
                       withAlpha(MUTAGEN_GLOW, 0.22f * (1.0f - t)));
    }
}

// ===================================================================================
// Element 3 (req 114): manhole covers and drain grates lying FLAT on the walkway surface
// (squashed circles = seen from the side), so characters simply walk over them.
// ===================================================================================
static void drawManhole(float x, int lane)
{
    glm::mat4 m = translate2D(glm::mat4(1.0f), x, walkwayY(lane) - 1.0f);
    const float w = 66.0f, h = 13.0f;
    Renderer::draw(Shape::Circle, part(m, 0.0f, -1.0f, w + 8.0f, h + 4.0f), hexColor(0x23272A));   // seat ring + shadow
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, w, h), hexColor(0x50565A));                 // cover
    Renderer::draw(Shape::Ring, part(m, 0.0f, 0.0f, w * 0.72f, h * 0.72f), hexColor(0x3B4044));    // inner groove
    // cast grip pattern: short bars across the cover
    for (int i = -2; i <= 2; ++i)
        Renderer::draw(Shape::Quad, part(m, i * 9.0f, 0.0f, 5.0f, h * 0.38f * (1.0f - 0.12f * std::abs(i))), hexColor(0x3B4044));
    Renderer::draw(Shape::Circle, part(m, -w * 0.32f, 1.0f, 4.0f, 1.8f), hexColor(0x15181A));    // pick holes
    Renderer::draw(Shape::Circle, part(m, w * 0.32f, 1.0f, 4.0f, 1.8f), hexColor(0x15181A));
    Renderer::draw(Shape::Quad, part(m, -10.0f, h * 0.36f, 22.0f, 1.2f), withAlpha(WHITE, 0.15f)); // worn shine
}

static void drawGrate(float x, int lane, float time)
{
    glm::mat4 m = translate2D(glm::mat4(1.0f), x, walkwayY(lane));
    const float w = 58.0f, h = 12.0f;
    Renderer::draw(Shape::Quad, part(m, 0.0f, 0.0f, w + 6.0f, h + 4.0f), hexColor(0x2A2E30));   // frame
    Renderer::draw(Shape::Quad, part(m, 0.0f, 0.0f, w, h), hexColor(0x0D100F));               // dark drain
    for (int i = 0; i < 7; ++i) {
        float sx = -w * 0.5f + 4.0f + i * (w - 8.0f) / 6.0f;
        Renderer::draw(Shape::Quad, part(m, sx + 1.5f, 0.0f, 3.0f, h, -12.0f), hexColor(0x4C5256));   // slanted bars
    }
    // grime streak on the curb below + water trickling down the front face
    float y0 = laneBottom(lane);
    Renderer::drawRect({ x, y0 + LEDGE_FACE * 0.5f }, { 26.0f, LEDGE_FACE }, withAlpha(hexColor(0x2E4A2A), 0.6f));
    float t = fmodf(time * 0.9f + x * 0.01f, 1.0f);
    Renderer::drawCircle({ x + 4.0f, y0 + LEDGE_FACE - 14.0f * t }, 2.0f, withAlpha(hexColor(0xA9C9BE), 0.7f * (1.0f - t)));
}

static void drawManholes(float time)
{
    drawManhole(1130.0f, 0);
    drawManhole(450.0f, 1);
    drawManhole(770.0f, 2);
    drawManhole(1330.0f, 3);
    drawGrate(860.0f, 0, time);
    drawGrate(1250.0f, 1, time);
    drawGrate(560.0f, 3, time);
}

// ===================================================================================
// Element 4 (req 115): hanging work lamps. Cord, metal shade, socket, bulb inside a wire cage,
// glow, a soft light cone down to the walkway and a light pool on it. They sway a little and
// flicker (one of them is dying and cuts out now and then).
// ===================================================================================
static void drawLamp(float x, float hangY, float cordLength, float floorY, float time, int index)
{
    float flicker = 0.88f + 0.12f * sinf(time * 13.0f + index * 1.7f) * sinf(time * 7.3f + index);
    if (index == 2 && fmodf(time, 5.3f) < 0.25f) flicker = 0.25f;   // the bad bulb
    float sway = 2.5f * sinf(time * 1.1f + index * 2.0f);

    glm::mat4 hang = joint(glm::mat4(1.0f), x, hangY, sway);
    glm::mat4 lamp = translate2D(hang, 0.0f, -cordLength);
    float coneLength = (hangY - cordLength) - floorY;

    // light first (behind the lamp): cone, inner cone, pool on the walkway
    Renderer::draw(Shape::Triangle, part(lamp, 0.0f, -coneLength * 0.5f - 8.0f, 150.0f, coneLength), withAlpha(LAMP_WARM, 0.055f * flicker));
    Renderer::draw(Shape::Triangle, part(lamp, 0.0f, -coneLength * 0.5f - 8.0f, 80.0f, coneLength), withAlpha(LAMP_WARM, 0.045f * flicker));
    Renderer::draw(Shape::Circle, part(glm::mat4(1.0f), x + sway * 0.8f, floorY, 170.0f, 14.0f), withAlpha(LAMP_WARM, 0.10f * flicker));
    Renderer::draw(Shape::Circle, part(lamp, 0.0f, -10.0f, 46.0f, 46.0f), withAlpha(LAMP_WARM, 0.12f * flicker));

    // cord, shade (trapezoid = quad + two triangles), socket, bulb, cage
    Renderer::draw(Shape::Quad, part(hang, 0.0f, -cordLength * 0.5f, 2.0f, cordLength), hexColor(0x1A1A1A));
    Renderer::draw(Shape::Quad, part(lamp, 0.0f, 2.0f, 8.0f, 6.0f), hexColor(0x2B2F2C));
    Renderer::draw(Shape::Quad, part(lamp, 0.0f, -3.0f, 14.0f, 6.0f), hexColor(0x5F6A55));
    Renderer::draw(Shape::RightTriangle, part(lamp, 8.5f, -3.0f, 5.0f, 6.0f, 90.0f), hexColor(0x5F6A55));
    Renderer::draw(Shape::RightTriangle, part(lamp, -8.5f, -3.0f, 5.0f, 6.0f, 0.0f), hexColor(0x5F6A55));
    Renderer::draw(Shape::Quad, part(lamp, 0.0f, -6.5f, 24.0f, 2.0f), hexColor(0x3E4638));
    Renderer::draw(Shape::Circle, part(lamp, 0.0f, -13.0f, 10.0f, 12.0f), glm::mix(hexColor(0x6B5A30), LAMP_WARM, flicker));
    Renderer::draw(Shape::Circle, part(lamp, -1.5f, -11.5f, 3.0f, 4.0f), withAlpha(WHITE, 0.7f * flicker));
    for (int i = -1; i <= 1; ++i)
        Renderer::draw(Shape::Quad, part(lamp, i * 6.0f, -13.5f, 1.2f, 15.0f), hexColor(0x2A2A2A));
    Renderer::draw(Shape::Quad, part(lamp, 0.0f, -13.0f, 15.0f, 1.2f), hexColor(0x2A2A2A));
    Renderer::draw(Shape::Quad, part(lamp, 0.0f, -21.0f, 9.0f, 1.5f), hexColor(0x2A2A2A));
}

static void drawWorkLamps(float time)
{
    // top lane: from the vault (the cord disappears under the HUD in game)
    drawLamp(FIRST_PILLAR_X + 1.5f * BAY_WIDTH, ARCH_SPRING_Y + ARCH_INNER, 44.0f, walkwayY(0), time, 0);
    drawLamp(FIRST_PILLAR_X + 3.5f * BAY_WIDTH, ARCH_SPRING_Y + ARCH_INNER, 44.0f, walkwayY(0), time, 1);
    // lower lanes: hung under the walkway above
    drawLamp(1160.0f, laneTop(1), 12.0f, walkwayY(1), time, 2);
    drawLamp(440.0f, laneTop(2), 12.0f, walkwayY(2), time, 3);
    drawLamp(820.0f, laneTop(3), 12.0f, walkwayY(3), time, 4);
}

// ===================================================================================
// Element 5 (req 116): graffiti on the brick wall (original tags, semi-transparent spray paint)
// and a bolted "LAIR" sign with an arrow pointing home.
// ===================================================================================
static void drawStroke(const glm::mat4& m, float x0, float y0, float x1, float y1, float width, const glm::vec4& color)
{
    float dx = x1 - x0, dy = y1 - y0;
    float length = sqrtf(dx * dx + dy * dy);
    Renderer::draw(Shape::Quad, part(m, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f, length, width, glm::degrees(atan2f(dy, dx))), color);
}

// blocky stencil letters, origin = bottom-left, size w x h
static void drawLetter(const glm::mat4& m, char c, float x, float y, float w, float h, float s, const glm::vec4& color)
{
    switch (c) {
    case 'L':
        drawStroke(m, x + s * 0.5f, y, x + s * 0.5f, y + h, s, color);
        drawStroke(m, x, y + s * 0.5f, x + w, y + s * 0.5f, s, color);
        break;
    case 'A':
        drawStroke(m, x + s * 0.5f, y, x + s * 0.5f, y + h, s, color);
        drawStroke(m, x + w - s * 0.5f, y, x + w - s * 0.5f, y + h, s, color);
        drawStroke(m, x, y + h - s * 0.5f, x + w, y + h - s * 0.5f, s, color);
        drawStroke(m, x, y + h * 0.5f, x + w, y + h * 0.5f, s, color);
        break;
    case 'I':
        drawStroke(m, x + w * 0.5f, y, x + w * 0.5f, y + h, s, color);
        break;
    case 'S':
        drawStroke(m, x, y + h - s * 0.5f, x + w, y + h - s * 0.5f, s, color);
        drawStroke(m, x, y + h * 0.5f, x + w, y + h * 0.5f, s, color);
        drawStroke(m, x, y + s * 0.5f, x + w, y + s * 0.5f, s, color);
        drawStroke(m, x + s * 0.5f, y + h * 0.5f, x + s * 0.5f, y + h, s, color);
        drawStroke(m, x + w - s * 0.5f, y, x + w - s * 0.5f, y + h * 0.5f, s, color);
        break;
    case 'H':
        drawStroke(m, x + s * 0.5f, y, x + s * 0.5f, y + h, s, color);
        drawStroke(m, x + w - s * 0.5f, y, x + w - s * 0.5f, y + h, s, color);
        drawStroke(m, x, y + h * 0.5f, x + w, y + h * 0.5f, s, color);
        break;
    case 'E':
        drawStroke(m, x + s * 0.5f, y, x + s * 0.5f, y + h, s, color);
        drawStroke(m, x, y + h - s * 0.5f, x + w, y + h - s * 0.5f, s, color);
        drawStroke(m, x, y + h * 0.5f, x + w * 0.8f, y + h * 0.5f, s, color);
        drawStroke(m, x, y + s * 0.5f, x + w, y + s * 0.5f, s, color);
        break;
    case 'R':
        drawStroke(m, x + s * 0.5f, y, x + s * 0.5f, y + h, s, color);
        drawStroke(m, x, y + h - s * 0.5f, x + w, y + h - s * 0.5f, s, color);
        drawStroke(m, x + w - s * 0.5f, y + h * 0.5f, x + w - s * 0.5f, y + h, s, color);
        drawStroke(m, x, y + h * 0.5f, x + w, y + h * 0.5f, s, color);
        drawStroke(m, x + w * 0.4f, y + h * 0.5f, x + w - s * 0.3f, y, s, color);
        break;
    default: break;
    }
}

static void drawPaintDrip(const glm::mat4& m, float x, float y, float length, const glm::vec4& color)
{
    Renderer::draw(Shape::Quad, part(m, x, y - length * 0.5f, 2.5f, length), color);
    Renderer::draw(Shape::Circle, part(m, x, y - length, 4.0f, 5.0f), color);
}

static void drawGraffitiWall(float /*time*/)
{
    const glm::mat4 I(1.0f);

    // tag 1 (lane 1): "SHELL" in chunky spray letters, dark outline pass then colour pass, drips + sparkle
    {
        glm::mat4 m = rotate2D(translate2D(I, 560.0f, laneBottom(1) + 74.0f), 6.0f);
        const glm::vec4 outline = hexColor(0x14141A, 0.55f), paint = hexColor(0xD2843A, 0.55f), shine = hexColor(0xF3D9A0, 0.4f);
        const char* word = "SHELL";
        for (int i = 0; i < 5; ++i) drawLetter(m, word[i], i * 31.0f, 0.0f, 24.0f, 42.0f, 12.0f, outline);
        for (int i = 0; i < 5; ++i) drawLetter(m, word[i], i * 31.0f + 2.0f, 2.0f, 20.0f, 38.0f, 7.0f, paint);
        drawPaintDrip(m, 12.0f, 3.0f, 16.0f, paint);
        drawPaintDrip(m, 72.0f, 3.0f, 24.0f, paint);
        drawPaintDrip(m, 128.0f, 3.0f, 12.0f, paint);
        Renderer::draw(Shape::Quad, part(m, 172.0f, 44.0f, 4.0f, 22.0f, 45.0f), shine);   // 4-point sparkle
        Renderer::draw(Shape::Quad, part(m, 172.0f, 44.0f, 4.0f, 22.0f, -45.0f), shine);
        drawStroke(m, -4.0f, -8.0f, 150.0f, -8.0f, 4.0f, outline);                         // underline swoosh
    }
    // tag 2 (lane 2): the turtles' own mark, a spray-painted turtle seen from above
    // (head, four flippers, tail, shell rim, hexagon plates)
    {
        glm::mat4 m = rotate2D(translate2D(I, 1100.0f, laneBottom(2) + 100.0f), -12.0f);
        const glm::vec4 paint = hexColor(0x5FB08A, 0.42f), dark = hexColor(0x14201A, 0.5f);
        Renderer::draw(Shape::Circle, part(m, 0.0f, 50.0f, 24.0f, 26.0f), paint);                     // head
        Renderer::draw(Shape::Circle, part(m, -6.0f, 54.0f, 4.0f, 4.0f), dark);                        // eyes
        Renderer::draw(Shape::Circle, part(m, 6.0f, 54.0f, 4.0f, 4.0f), dark);
        for (int side = -1; side <= 1; side += 2) {
            Renderer::draw(Shape::Circle, part(m, side * 42.0f, 22.0f, 34.0f, 16.0f, side * 35.0f), paint);     // front flippers
            Renderer::draw(Shape::Circle, part(m, side * 36.0f, -26.0f, 24.0f, 13.0f, -side * 35.0f), paint);   // back flippers
        }
        Renderer::draw(Shape::Triangle, part(m, 0.0f, -44.0f, 10.0f, 14.0f, 180.0f), paint);           // tail
        Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 80.0f, 88.0f), dark);
        Renderer::draw(Shape::Ring, part(m, 0.0f, 0.0f, 76.0f, 84.0f), paint);
        Renderer::draw(g_hexagon, scale2D(m, 26.0f, 26.0f), paint);
        for (int i = 0; i < 6; ++i) {
            float a = glm::radians(60.0f * i);
            drawStroke(m, 13.0f * cosf(a), 13.0f * sinf(a), 32.0f * cosf(a), 35.0f * sinf(a), 3.0f, paint);
        }
        drawPaintDrip(m, -20.0f, -38.0f, 20.0f, paint);
        drawPaintDrip(m, 14.0f, -40.0f, 12.0f, paint);
    }
    // tag 3 (lane 3): jagged zig-zag tag + arrows pointing to the Lair
    {
        glm::mat4 m = translate2D(I, 1180.0f, laneBottom(3) + 110.0f);
        const glm::vec4 paint = hexColor(0x9C6BC2, 0.38f);
        const float zig[][2] = { { -80, -10 }, { -50, 20 }, { -30, -14 }, { 0, 22 }, { 20, -12 }, { 50, 18 }, { 76, -8 } };
        for (int i = 0; i + 1 < 7; ++i) drawStroke(m, zig[i][0], zig[i][1], zig[i + 1][0], zig[i + 1][1], 6.0f, paint);
        drawPaintDrip(m, -30.0f, -16.0f, 16.0f, paint);
        drawPaintDrip(m, 20.0f, -14.0f, 22.0f, paint);
        for (int k = 0; k < 3; ++k) {   // <<< chevrons
            float x = -140.0f - k * 26.0f;
            drawStroke(m, x, 0.0f, x + 14.0f, 14.0f, 5.0f, paint);
            drawStroke(m, x, 0.0f, x + 14.0f, -14.0f, 5.0f, paint);
        }
    }
    // tag 4 (lane 0, under an arch): an X splat with a ring around it
    {
        glm::mat4 m = translate2D(I, 1000.0f, laneBottom(0) + 104.0f);
        const glm::vec4 paint = hexColor(0xB8484A, 0.32f);
        drawStroke(m, -22.0f, -22.0f, 22.0f, 22.0f, 7.0f, paint);
        drawStroke(m, -22.0f, 22.0f, 22.0f, -22.0f, 7.0f, paint);
        Renderer::draw(Shape::Ring, part(m, 0.0f, 0.0f, 64.0f, 64.0f), withAlpha(paint, 0.22f));
        drawPaintDrip(m, 18.0f, -24.0f, 14.0f, paint);
    }

    // the Lair sign (lane 1, right next to the Lair): riveted plate, stencil LAIR, arrow home
    {
        glm::mat4 m = translate2D(I, 300.0f, laneTop(1) - 44.0f);
        Renderer::draw(Shape::Quad, part(m, -30.0f, 22.0f, 2.0f, 16.0f, 12.0f), hexColor(0x222222));   // hanging wires
        Renderer::draw(Shape::Quad, part(m, 30.0f, 22.0f, 2.0f, 16.0f, -12.0f), hexColor(0x222222));
        Renderer::draw(Shape::Quad, part(m, 1.5f, -1.5f, 104.0f, 38.0f), hexColor(0x101412, 0.6f));    // drop shadow
        Renderer::draw(Shape::Quad, part(m, 0.0f, 0.0f, 104.0f, 38.0f), hexColor(0x4E534C));
        Renderer::draw(Shape::Quad, part(m, 0.0f, 0.0f, 98.0f, 32.0f), hexColor(0x5E6459));
        Renderer::draw(Shape::Quad, part(m, 0.0f, 14.0f, 98.0f, 3.0f), hexColor(0x737A6C));
        for (int i = 0; i < 4; ++i) {   // corner rivets
            float rx = (i % 2) ? 47.0f : -47.0f, ry = (i / 2) ? 15.0f : -15.0f;
            Renderer::drawCircle({ 300.0f + rx, laneTop(1) - 44.0f + ry }, 2.0f, hexColor(0x8E9488));
        }
        const glm::vec4 stencil = hexColor(0xC9B26A, 0.85f);
        // arrow pointing left, to the Lair
        Renderer::draw(Shape::Triangle, part(m, -38.0f, 0.0f, 16.0f, 14.0f, 90.0f), stencil);
        Renderer::draw(Shape::Quad, part(m, -27.0f, 0.0f, 10.0f, 5.0f), stencil);
        const char* word = "LAIR";
        for (int i = 0; i < 4; ++i) drawLetter(m, word[i], -16.0f + i * 15.0f, -10.0f, 11.0f, 20.0f, 3.5f, stencil);
        Renderer::draw(Shape::Circle, part(m, 34.0f, 3.0f, 9.0f, 6.0f), withAlpha(RUST, 0.5f));     // rust stain
        Renderer::draw(Shape::Quad, part(m, 34.0f, -8.0f, 3.0f, 16.0f), withAlpha(RUST, 0.35f));
    }
}

// ===================================================================================
// Element 6 (req 117): stacked pizza boxes and wooden crates next to the Lair (turtle home stuff).
// ===================================================================================
static void drawPizzaBox(const glm::mat4& m, float x, float y, float tilt, bool open)
{
    glm::mat4 box = rotate2D(translate2D(m, x, y), tilt);
    const float w = 56.0f, h = 10.0f;
    Renderer::draw(Shape::Quad, part(box, 0.0f, 0.0f, w, h), CARDBOARD);
    Renderer::draw(Shape::Quad, part(box, 0.0f, h * 0.5f - 1.0f, w, 2.0f), shade(CARDBOARD, 1.15f));      // lid edge
    Renderer::draw(Shape::Quad, part(box, 0.0f, -h * 0.5f + 1.0f, w, 2.0f), shade(CARDBOARD, 0.7f));
    Renderer::draw(Shape::Quad, part(box, -w * 0.25f, 0.0f, 12.0f, 3.0f), hexColor(0x8A3A2E));      // printed stripe
    Renderer::draw(Shape::Circle, part(box, w * 0.3f, -1.0f, 9.0f, 4.0f), withAlpha(hexColor(0x5A3A1A), 0.5f));   // grease stain
    if (open) {   // lid ajar: a leftover crust sticking out of the gap
        Renderer::draw(Shape::Quad, part(box, 0.0f, h * 0.5f + 3.0f, w, 2.5f, 4.0f), shade(CARDBOARD, 0.9f));
        Renderer::draw(Shape::Quad, part(box, 22.0f, h * 0.5f + 1.0f, 18.0f, 4.0f, -8.0f), hexColor(0xB5762A));
        Renderer::draw(Shape::Circle, part(box, 31.0f, h * 0.5f, 5.0f, 5.0f), hexColor(0xB5762A));
    }
}

static void drawCrate(const glm::mat4& m, float x, float y, float size)
{
    glm::mat4 c = translate2D(m, x, y + size * 0.5f);
    Renderer::draw(Shape::Quad, part(c, 0.0f, 0.0f, size, size), WOOD);
    for (int i = -1; i <= 1; ++i)   // plank lines
        Renderer::draw(Shape::Quad, part(c, 0.0f, i * size * 0.25f, size, 1.5f), shade(WOOD, 0.6f));
    Renderer::draw(Shape::Quad, part(c, 0.0f, size * 0.5f - 3.0f, size, 6.0f), shade(WOOD, 1.25f));     // frame
    Renderer::draw(Shape::Quad, part(c, 0.0f, -size * 0.5f + 3.0f, size, 6.0f), shade(WOOD, 1.1f));
    Renderer::draw(Shape::Quad, part(c, -size * 0.5f + 3.0f, 0.0f, 6.0f, size), shade(WOOD, 1.15f));
    Renderer::draw(Shape::Quad, part(c, size * 0.5f - 3.0f, 0.0f, 6.0f, size), shade(WOOD, 0.85f));
    Renderer::draw(Shape::Quad, part(c, 0.0f, 0.0f, size * 1.25f, 6.0f, 45.0f), shade(WOOD, 1.05f));    // diagonal brace
    for (int i = 0; i < 4; ++i) {
        float nx = (i % 2) ? size * 0.5f - 3.0f : -size * 0.5f + 3.0f, ny = (i / 2) ? size * 0.5f - 3.0f : -size * 0.5f + 3.0f;
        Renderer::draw(Shape::Circle, part(c, nx, ny, 2.5f, 2.5f), hexColor(0x2A2A2A));
    }
}

static void drawPizzaBoxes(float /*time*/)
{
    const glm::mat4 I(1.0f);
    float x0 = LAIR_WIDTH + 30.0f;
    // lane 3: a leaning tower of boxes, the top one open
    float y = laneBottom(3) + LEDGE_BACK - 2.0f;
    const float tilts[] = { 0.0f, 2.0f, -3.0f, 4.0f, -2.0f };
    for (int i = 0; i < 5; ++i) drawPizzaBox(I, x0 + 2.0f * i * (i % 2 ? 1.0f : -0.5f), y + 5.0f + i * 10.0f, tilts[i], i == 4);
    // lane 1: crate with two boxes on top
    y = laneBottom(1) + LEDGE_BACK - 2.0f;
    drawCrate(I, x0, y, 40.0f);
    drawPizzaBox(I, x0 + 2.0f, y + 45.0f, -2.0f, false);
    drawPizzaBox(I, x0 - 3.0f, y + 55.0f, 3.0f, false);
    // lane 2: two crates, small one on top
    y = laneBottom(2) + LEDGE_BACK - 2.0f;
    drawCrate(I, x0 - 4.0f, y, 38.0f);
    drawCrate(I, x0 + 6.0f, y + 38.0f, 26.0f);
}

// Draw order (back to front): wall art, pipes, lamps, barrels, flat things on the walkways, boxes.
void Environment::drawProps(float time)
{
    drawGraffitiWall(time);
    drawPipes(time);
    drawWorkLamps(time);
    drawMutagenBarrels(time);
    drawManholes(time);
    drawPizzaBoxes(time);
}
