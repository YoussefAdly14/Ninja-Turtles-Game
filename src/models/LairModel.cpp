#include "models/LairModel.h"
#include "core/Renderer.h"
#include "core/Mesh.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "Config.h"
#include <cmath>
#include <vector>

// The turtles' Lair, home base at the left edge across every lane (req 40).
// It is a riveted STEEL bulkhead (the sewer around it is green-grey brick) with a big round vault
// door carrying a turtle-shell emblem, a hazard-striped edge facing the lanes, pipes, gauges, lights
// and the mutagen glowing from inside (door seam, porthole with the canister, vent slats).
// Static parts are per-vertex-coloured meshes built once in init() (Lab 3 one VBO + EBO, Lab 2
// colour interpolation); moving lights are building-block shapes changed by uniforms only.

// ---------------- layout (world units) ----------------
const float STRIPE_WIDTH = 16.0f;                         // hazard column on the lane side
const float FACADE_RIGHT = LAIR_WIDTH - STRIPE_WIDTH;     // 154
const float LAIR_BOTTOM = 28.0f;                          // the foundation stands in the water channel
const float DOOR_X = FACADE_RIGHT * 0.5f;
const float DOOR_Y = PLAYFIELD_BOTTOM + (PLAYFIELD_TOP - PLAYFIELD_BOTTOM) * 0.5f;
const float FRAME_OUTER = 76.0f, FRAME_INNER = 64.0f;
const float DOOR_RADIUS = 61.0f;
const float PORTHOLE_Y = 618.0f;
const float PI_F = 3.14159265f;

// ---------------- colours ----------------
const glm::vec4 STEEL       = hexColor(0x434C55);
const glm::vec4 STEEL_DARK  = hexColor(0x272D33);
const glm::vec4 STEEL_LIGHT = hexColor(0x7C868F);
const glm::vec4 HAZARD      = hexColor(0xC29B2A);
const glm::vec4 SHELL_DARK  = hexColor(0x4A361B);
const glm::vec4 SHELL_LIGHT = hexColor(0x9C7B45);
const glm::vec4 BULB_WARM   = hexColor(0xFFC857);
const glm::vec4 PIPE_COPPER = hexColor(0x7A5A3E);

static Mesh g_plates;      // riveted steel plates of the facade
static Mesh g_stripes;     // yellow/black hazard column
static Mesh g_frame;       // door frame ring (bevelled)
static Mesh g_doorFace;    // door face, lighter in the middle
static Mesh g_thinRing;    // seams, grooves, porthole rim (outer radius 0.5)
static Mesh g_hexagon;     // shell scutes

struct MeshBuilder
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    void vertex(glm::vec2 p, glm::vec3 c) { vertices.insert(vertices.end(), { p.x, p.y, 0.0f, c.r, c.g, c.b }); }
    void quad(glm::vec2 bl, glm::vec2 br, glm::vec2 tr, glm::vec2 tl,
              glm::vec3 cbl, glm::vec3 cbr, glm::vec3 ctr, glm::vec3 ctl)
    {
        unsigned int i = static_cast<unsigned int>(vertices.size() / 6);
        vertex(bl, cbl); vertex(br, cbr); vertex(tr, ctr); vertex(tl, ctl);
        indices.insert(indices.end(), { i, i + 1, i + 2,   i + 2, i + 3, i });
    }
    void rect(float x0, float y0, float x1, float y1, glm::vec3 bottom, glm::vec3 top)
    {
        quad({ x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 }, bottom, bottom, top, top);
    }
    // ring around (0,0): one quad per segment, colour from inner to outer edge
    void ring(float inner, float outer, int segments, glm::vec3 cInner, glm::vec3 cOuter)
    {
        for (int i = 0; i < segments; ++i) {
            float a0 = 2.0f * PI_F * i / segments, a1 = 2.0f * PI_F * (i + 1) / segments;
            quad({ inner * cosf(a0), inner * sinf(a0) }, { inner * cosf(a1), inner * sinf(a1) },
                 { outer * cosf(a1), outer * sinf(a1) }, { outer * cosf(a0), outer * sinf(a0) },
                 cInner, cInner, cOuter, cOuter);
        }
    }
    Mesh build() const
    {
        return createIndexedMesh(vertices.data(), vertices.size() * sizeof(float),
                                 indices.data(), indices.size() * sizeof(unsigned int));
    }
};

static glm::vec3 rgb(const glm::vec4& c) { return glm::vec3(c); }
static glm::vec4 shade(const glm::vec4& c, float k) { return glm::vec4(glm::vec3(c) * k, c.a); }

static Mesh buildPlates()
{
    MeshBuilder b;
    b.rect(0.0f, LAIR_BOTTOM, FACADE_RIGHT, WORLD_HEIGHT, rgb(STEEL_DARK) * 0.6f, rgb(STEEL_DARK) * 0.6f);   // seams
    const float plateH = 116.0f, gap = 3.0f;
    const float colX[3] = { 0.0f, FACADE_RIGHT * 0.5f, FACADE_RIGHT };
    int row = 0;
    for (float y = LAIR_BOTTOM; y < WORLD_HEIGHT; y += plateH, ++row) {
        for (int c = 0; c < 2; ++c) {
            float x0 = colX[c] + gap * 0.5f, x1 = colX[c + 1] - gap * 0.5f;
            float y0 = y + gap * 0.5f, y1 = fminf(y + plateH, WORLD_HEIGHT) - gap * 0.5f;
            float k = 0.9f + 0.12f * ((row + c) % 2);
            // brushed steel: lighter top-left, darker bottom-right
            b.quad({ x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 },
                   rgb(STEEL) * k * 0.8f, rgb(STEEL) * k * 0.7f, rgb(STEEL) * k * 0.95f, rgb(STEEL) * k * 1.15f);
            // rivets along the plate edges: small diamonds with a light top half
            for (float rx = x0 + 7.0f; rx < x1 - 4.0f; rx += 14.0f) {
                for (float ry : { y0 + 5.0f, y1 - 5.0f }) {
                    b.quad({ rx, ry - 2.2f }, { rx + 2.2f, ry }, { rx, ry + 2.2f }, { rx - 2.2f, ry },
                           rgb(STEEL_DARK), rgb(STEEL_DARK), rgb(STEEL_LIGHT), rgb(STEEL_LIGHT));
                }
            }
        }
    }
    return b.build();
}

static Mesh buildStripes()
{
    MeshBuilder b;
    const float x0 = FACADE_RIGHT, x1 = LAIR_WIDTH;
    b.rect(x0, LAIR_BOTTOM, x1, WORLD_HEIGHT, glm::vec3(0.08f), glm::vec3(0.1f));
    // diagonal bands, rising towards the lanes
    for (float y = LAIR_BOTTOM - 16.0f; y < WORLD_HEIGHT; y += 28.0f) {
        float yb = fmaxf(y, LAIR_BOTTOM);
        b.quad({ x0, yb }, { x1, yb + 16.0f }, { x1, y + 30.0f }, { x0, y + 14.0f },
               rgb(HAZARD) * 0.85f, rgb(HAZARD) * 0.85f, rgb(HAZARD), rgb(HAZARD));
    }
    b.rect(x0, LAIR_BOTTOM, x0 + 2.0f, WORLD_HEIGHT, rgb(STEEL_DARK) * 0.5f, rgb(STEEL_DARK) * 0.5f);
    b.rect(x1 - 3.0f, LAIR_BOTTOM, x1, WORLD_HEIGHT, rgb(STEEL_LIGHT) * 0.8f, rgb(STEEL_LIGHT) * 0.8f);   // lit edge
    return b.build();
}

void LairModel::init()
{
    g_plates = buildPlates();
    g_stripes = buildStripes();

    MeshBuilder frame;
    frame.ring(FRAME_INNER, FRAME_OUTER, 48, rgb(STEEL_LIGHT), rgb(STEEL_DARK));    // bevel: bright inside edge
    frame.ring(FRAME_OUTER, FRAME_OUTER + 3.0f, 48, rgb(STEEL_DARK) * 0.5f, rgb(STEEL_DARK) * 0.5f);
    g_frame = frame.build();

    // door face: Lab 3 circle (centre + rim, triangle fan) with a lighter centre vertex
    std::vector<float> face = { 0.0f, 0.0f, 0.0f, 0.55f, 0.59f, 0.63f };
    for (int i = 0; i <= 48; ++i) {
        float a = 2.0f * PI_F * i / 48;
        face.insert(face.end(), { DOOR_RADIUS * cosf(a), DOOR_RADIUS * sinf(a), 0.0f, 0.30f, 0.34f, 0.38f });
    }
    g_doorFace = createMesh(face.data(), face.size() * sizeof(float), GL_TRIANGLE_FAN);

    MeshBuilder ring;
    ring.ring(0.44f, 0.5f, 40, glm::vec3(1.0f), glm::vec3(1.0f));
    g_thinRing = ring.build();

    std::vector<glm::vec2> hex;
    for (int i = 0; i < 6; ++i) {
        float a = glm::radians(90.0f + 60.0f * i);
        hex.push_back({ 0.5f * cosf(a), 0.5f * sinf(a) });
    }
    g_hexagon = createPolygonMesh(hex);
}

void LairModel::shutdown()
{
    destroyMesh(g_plates);
    destroyMesh(g_stripes);
    destroyMesh(g_frame);
    destroyMesh(g_doorFace);
    destroyMesh(g_thinRing);
    destroyMesh(g_hexagon);
}

// ---------------- parts ----------------
static void drawVerticalPipe(float x, float y0, float y1, float w)
{
    Renderer::drawRect({ x, (y0 + y1) * 0.5f }, { w, y1 - y0 }, PIPE_COPPER);
    Renderer::drawRect({ x - w * 0.25f, (y0 + y1) * 0.5f }, { w * 0.2f, y1 - y0 }, shade(PIPE_COPPER, 1.35f));
    Renderer::drawRect({ x + w * 0.33f, (y0 + y1) * 0.5f }, { w * 0.22f, y1 - y0 }, shade(PIPE_COPPER, 0.6f));
}

static void drawPipeFlange(float x, float y, float w)
{
    Renderer::drawRect({ x, y }, { w + 8.0f, 7.0f }, STEEL_DARK);
    Renderer::drawRect({ x, y + 1.5f }, { w + 8.0f, 2.0f }, STEEL_LIGHT);
}

// Shell emblem on the door: oval shell, rim, marginal seam, three vertebral scutes, costal seams
static void drawShellEmblem(const glm::mat4& m)
{
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 66.0f, 78.0f), SHELL_DARK);
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 60.0f, 72.0f), SHELL);
    Renderer::draw(g_thinRing, part(m, 0.0f, 0.0f, 50.0f, 62.0f), SHELL_DARK);
    // costal seams: from the scute column out to the marginal seam
    for (int side = -1; side <= 1; side += 2) {
        Renderer::draw(Shape::Quad, part(m, side * 16.0f, 10.0f, 14.0f, 2.2f, side * 12.0f), SHELL_DARK);
        Renderer::draw(Shape::Quad, part(m, side * 16.0f, -10.0f, 14.0f, 2.2f, -side * 12.0f), SHELL_DARK);
    }
    for (int i = -1; i <= 1; ++i) {
        float y = i * 19.0f;
        Renderer::draw(g_hexagon, scale2D(translate2D(m, 0.0f, y), 21.0f, 21.0f), SHELL_DARK);
        Renderer::draw(g_hexagon, scale2D(translate2D(m, 0.0f, y), 16.5f, 16.5f), SHELL_LIGHT);
    }
    Renderer::draw(Shape::Circle, part(m, -14.0f, 20.0f, 9.0f, 14.0f, 25.0f), withAlpha(WHITE, 0.15f));   // shine
}

static void drawVaultDoor(float time)
{
    glm::mat4 m = translate2D(glm::mat4(1.0f), DOOR_X, DOOR_Y);
    float pulse = 0.5f + 0.5f * sinf(time * 2.0f);

    // mutagen light leaking around the door onto the bulkhead
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 2.0f * FRAME_OUTER + 4.0f, 2.0f * FRAME_OUTER + 4.0f), withAlpha(MUTAGEN_GLOW, 0.08f + 0.08f * pulse));
    Renderer::draw(g_frame, m, WHITE);
    for (int i = 0; i < 16; ++i) {   // frame bolts
        float a = 2.0f * PI_F * (i + 0.5f) / 16.0f;
        float r = (FRAME_INNER + FRAME_OUTER) * 0.5f;
        Renderer::draw(Shape::Circle, part(m, r * cosf(a), r * sinf(a), 5.0f, 5.0f), STEEL_DARK);
        Renderer::draw(Shape::Circle, part(m, r * cosf(a) - 0.6f, r * sinf(a) + 0.6f, 3.0f, 3.0f), STEEL_LIGHT);
    }
    // glowing seam between frame and door
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 2.0f * FRAME_INNER, 2.0f * FRAME_INNER), glm::mix(hexColor(0x2F6B12), MUTAGEN_GLOW, 0.4f + 0.6f * pulse));
    Renderer::draw(g_doorFace, m, WHITE);
    Renderer::draw(g_thinRing, part(m, 0.0f, 0.0f, 2.0f * DOOR_RADIUS, 2.0f * DOOR_RADIUS), STEEL_DARK);
    Renderer::draw(g_thinRing, part(m, 0.0f, 0.0f, 100.0f, 100.0f), withAlpha(STEEL_DARK, 0.8f));       // groove
    Renderer::draw(g_thinRing, part(m, 0.0f, 0.0f, 92.0f, 92.0f), withAlpha(STEEL_LIGHT, 0.5f));

    // locking bars sliding from the door into the frame (right side) + hinges (left side)
    for (int i = -1; i <= 1; ++i) {
        glm::mat4 bar = rotate2D(m, i * 32.0f);
        Renderer::draw(Shape::Quad, part(bar, DOOR_RADIUS + 2.0f, 0.0f, 22.0f, 9.0f), STEEL_DARK);
        Renderer::draw(Shape::Quad, part(bar, DOOR_RADIUS + 2.0f, 1.5f, 22.0f, 3.0f), STEEL_LIGHT);
        Renderer::draw(Shape::Quad, part(bar, DOOR_RADIUS - 12.0f, 0.0f, 8.0f, 13.0f), STEEL);       // bar housing
    }
    for (int i = -1; i <= 1; i += 2) {
        Renderer::draw(Shape::Quad, part(m, -DOOR_RADIUS - 3.0f, i * 36.0f, 16.0f, 22.0f), STEEL_DARK);
        Renderer::draw(Shape::Quad, part(m, -DOOR_RADIUS - 3.0f, i * 36.0f, 6.0f, 26.0f), STEEL_LIGHT);
    }
    drawShellEmblem(m);
}

// Round porthole above the door: the last mutagen canister, glowing, bubbles rising (story!)
static void drawPorthole(float time)
{
    glm::mat4 m = translate2D(glm::mat4(1.0f), DOOR_X, PORTHOLE_Y);
    float pulse = 0.5f + 0.5f * sinf(time * 2.6f);

    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 80.0f, 80.0f), STEEL_DARK);
    Renderer::draw(Shape::Ring, part(m, 0.0f, 0.0f, 76.0f, 76.0f), STEEL_LIGHT);
    for (int i = 0; i < 8; ++i) {
        float a = 2.0f * PI_F * i / 8.0f;
        Renderer::draw(Shape::Circle, part(m, 34.0f * cosf(a), 34.0f * sinf(a), 4.0f, 4.0f), STEEL_DARK);
    }
    // inside: dark green room lit by the canister
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 58.0f, 58.0f), hexColor(0x0A1F10));
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 50.0f, 50.0f), withAlpha(MUTAGEN_GLOW, 0.15f + 0.15f * pulse));
    // canister: caps, glass tube, glowing ooze, bubbles
    Renderer::draw(Shape::Quad, part(m, 0.0f, 0.0f, 18.0f, 34.0f), withAlpha(hexColor(0xBFE8B0), 0.35f));
    Renderer::draw(Shape::Quad, part(m, 0.0f, -3.0f, 14.0f, 26.0f), glm::mix(hexColor(0x4FB51A), MUTAGEN_GLOW, pulse));
    Renderer::draw(Shape::Quad, part(m, 0.0f, 18.0f, 22.0f, 6.0f), hexColor(0x8E979E));
    Renderer::draw(Shape::Quad, part(m, 0.0f, -18.0f, 22.0f, 6.0f), hexColor(0x8E979E));
    Renderer::draw(Shape::Quad, part(m, 0.0f, 22.0f, 8.0f, 3.0f), hexColor(0x5C646B));
    for (int i = 0; i < 3; ++i) {
        float t = fmodf(time * 0.7f + i / 3.0f, 1.0f);
        Renderer::draw(Shape::Circle, part(m, -3.0f + 3.0f * i + sinf(time * 4.0f + i) * 1.5f, -14.0f + 24.0f * t, 3.0f, 3.0f), withAlpha(WHITE, 0.6f * (1.0f - t)));
    }
    Renderer::draw(Shape::Quad, part(m, -5.0f, 0.0f, 2.0f, 28.0f), withAlpha(WHITE, 0.4f));
    // glass reflection
    Renderer::draw(Shape::Quad, part(m, -12.0f, 12.0f, 20.0f, 3.0f, 45.0f), withAlpha(WHITE, 0.22f));
}

// Marquee bulbs around the top of the door (a chasing light) + two blinking beacons up top
static void drawLights(float time)
{
    const int bulbs = 7;
    int lit = static_cast<int>(time * 5.0f) % bulbs;
    for (int i = 0; i < bulbs; ++i) {
        float a = glm::radians(30.0f + 120.0f * i / (bulbs - 1));
        glm::vec2 p(DOOR_X + 88.0f * cosf(a), DOOR_Y + 88.0f * sinf(a));
        bool on = (i == lit) || (i == (lit + 3) % bulbs);
        Renderer::drawRect(p - glm::vec2(0.0f, 5.0f), { 6.0f, 5.0f }, STEEL_DARK);
        if (on) Renderer::drawCircle(p, 10.0f, withAlpha(BULB_WARM, 0.3f));
        Renderer::drawCircle(p, 4.5f, on ? BULB_WARM : shade(BULB_WARM, 0.45f));
    }
    for (int i = 0; i < 2; ++i) {
        float x = (i == 0) ? 40.0f : FACADE_RIGHT - 40.0f;
        float y = 724.0f;
        bool on = fmodf(time + i * 0.5f, 1.0f) < 0.5f;
        Renderer::drawRect({ x, y - 4.0f }, { 26.0f, 8.0f }, STEEL_DARK);
        Renderer::draw(Shape::Semicircle, part(glm::mat4(1.0f), x, y, 20.0f, 20.0f), on ? hexColor(0xFF9A2E) : hexColor(0x6E3F16));
        Renderer::draw(Shape::Quad, part(glm::mat4(1.0f), x, y + 4.0f, 2.0f, 8.0f), withAlpha(WHITE, 0.3f));
        if (on) Renderer::drawCircle({ x, y + 4.0f }, 18.0f, hexColor(0xFF9A2E, 0.2f));
    }
}

// Pipes into the bulkhead: copper risers with flanges, a crossover with a pressure gauge, a valve
static void drawLairPipes(float time)
{
    // upper risers from the porthole level into the ceiling, crossover at 690
    drawVerticalPipe(18.0f, 560.0f, WORLD_HEIGHT, 14.0f);
    drawVerticalPipe(FACADE_RIGHT - 18.0f, 660.0f, WORLD_HEIGHT, 14.0f);
    Renderer::drawRect({ FACADE_RIGHT * 0.5f, 690.0f }, { FACADE_RIGHT - 36.0f, 12.0f }, PIPE_COPPER);
    Renderer::drawRect({ FACADE_RIGHT * 0.5f, 693.0f }, { FACADE_RIGHT - 36.0f, 2.5f }, shade(PIPE_COPPER, 1.35f));
    drawPipeFlange(18.0f, 600.0f, 14.0f);
    drawPipeFlange(18.0f, 740.0f, 14.0f);
    drawPipeFlange(FACADE_RIGHT - 18.0f, 740.0f, 14.0f);
    // pressure gauge on the crossover
    glm::mat4 g = translate2D(glm::mat4(1.0f), FACADE_RIGHT * 0.5f, 690.0f);
    Renderer::draw(Shape::Circle, part(g, 0.0f, 0.0f, 24.0f, 24.0f), STEEL_DARK);
    Renderer::draw(Shape::Circle, part(g, 0.0f, 0.0f, 18.0f, 18.0f), hexColor(0xD9D3B8));
    Renderer::draw(Shape::Quad, part(joint(g, 0.0f, 0.0f, 30.0f * sinf(time * 0.8f) - 20.0f), 0.0f, 3.5f, 1.6f, 8.0f), hexColor(0xA03A30));
    // lower riser out of the foundation, elbow into the bulkhead, with a valve wheel
    drawVerticalPipe(18.0f, LAIR_BOTTOM + 30.0f, 300.0f, 14.0f);
    Renderer::drawRect({ 30.0f, 300.0f }, { 24.0f, 14.0f }, PIPE_COPPER);
    drawPipeFlange(18.0f, 120.0f, 14.0f);
    glm::mat4 v = rotate2D(translate2D(glm::mat4(1.0f), 18.0f, 230.0f), 15.0f * sinf(time * 0.5f));
    for (int i = 0; i < 3; ++i)
        Renderer::draw(Shape::Quad, part(rotate2D(v, 60.0f * i), 0.0f, 0.0f, 24.0f, 2.5f), hexColor(0xA03A30));
    Renderer::draw(Shape::Ring, part(v, 0.0f, 0.0f, 28.0f, 28.0f), hexColor(0xB04536));
    Renderer::draw(Shape::Circle, part(v, 0.0f, 0.0f, 7.0f, 7.0f), STEEL_LIGHT);
}

// Vent grille below the door, mutagen glow leaking between the slats; keypad by the door
static void drawVentAndKeypad(float time)
{
    float pulse = 0.5f + 0.5f * sinf(time * 2.0f + 1.0f);
    glm::mat4 v = translate2D(glm::mat4(1.0f), DOOR_X + 6.0f, 190.0f);
    Renderer::draw(Shape::Quad, part(v, 0.0f, 0.0f, 96.0f, 70.0f), STEEL_DARK);
    Renderer::draw(Shape::Quad, part(v, 0.0f, 0.0f, 88.0f, 62.0f), glm::mix(hexColor(0x1E4A10), MUTAGEN_GLOW, 0.25f + 0.35f * pulse));
    for (int i = 0; i < 6; ++i) {
        float y = -26.0f + i * 10.4f;
        Renderer::draw(Shape::Quad, part(v, 0.0f, y, 88.0f, 6.0f), STEEL);
        Renderer::draw(Shape::Quad, part(v, 0.0f, y + 2.0f, 88.0f, 1.5f), STEEL_LIGHT);
    }
    for (int i = 0; i < 4; ++i)
        Renderer::draw(Shape::Circle, part(v, (i % 2) ? 42.0f : -42.0f, (i / 2) ? 29.0f : -29.0f, 4.0f, 4.0f), STEEL_LIGHT);

    // keypad: housing, 3x3 keys, status LED
    glm::mat4 k = translate2D(glm::mat4(1.0f), FACADE_RIGHT - 22.0f, 300.0f);
    Renderer::draw(Shape::Quad, part(k, 0.0f, 0.0f, 24.0f, 34.0f), STEEL_DARK);
    Renderer::draw(Shape::Quad, part(k, 0.0f, 0.0f, 20.0f, 30.0f), hexColor(0x2E3439));
    for (int i = 0; i < 9; ++i)
        Renderer::draw(Shape::Quad, part(k, -6.0f + 6.0f * (i % 3), -8.0f + 6.0f * (i / 3), 4.0f, 4.0f), hexColor(0x8E979E));
    Renderer::draw(Shape::Circle, part(k, 0.0f, 11.0f, 4.0f, 4.0f), MUTAGEN_GLOW);
}

// One status light per lane on the hazard column: every lane leads home (req 40 "spans all lanes")
static void drawLaneLights(float time)
{
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        float pulse = 0.5f + 0.5f * sinf(time * 2.5f + lane * 1.3f);
        float laneCenter = PLAYFIELD_TOP - (lane + 0.5f) * LANE_HEIGHT;   // same as laneCenterY (models don't include game/)
        glm::vec2 p(FACADE_RIGHT + STRIPE_WIDTH * 0.5f, laneCenter + 30.0f);
        Renderer::drawRect(p, { 13.0f, 22.0f }, hexColor(0x15181A));
        Renderer::drawCircle(p, 4.5f, glm::mix(hexColor(0x2F6B12), MUTAGEN_GLOW, pulse));
        Renderer::drawCircle(p, 8.0f, withAlpha(MUTAGEN_GLOW, 0.15f * pulse));
    }
}

static void drawFoundation()
{
    // concrete footing in the channel, wet dark band at the waterline
    Renderer::drawRect({ LAIR_WIDTH * 0.5f, (LAIR_BOTTOM + PLAYFIELD_BOTTOM) * 0.5f }, { LAIR_WIDTH + 6.0f, PLAYFIELD_BOTTOM - LAIR_BOTTOM }, hexColor(0x4A4E48));
    Renderer::drawRect({ LAIR_WIDTH * 0.5f, PLAYFIELD_BOTTOM - 2.0f }, { LAIR_WIDTH + 6.0f, 4.0f }, hexColor(0x63685F));
    Renderer::drawRect({ LAIR_WIDTH * 0.5f, LAIR_BOTTOM + 6.0f }, { LAIR_WIDTH + 6.0f, 12.0f }, hexColor(0x1F2A26));
    for (float x = 30.0f; x < LAIR_WIDTH; x += 56.0f)
        Renderer::drawRect({ x, (LAIR_BOTTOM + PLAYFIELD_BOTTOM) * 0.5f + 4.0f }, { 2.0f, 20.0f }, hexColor(0x30342F));
}

void LairModel::draw(float time)
{
    Renderer::draw(g_plates, glm::mat4(1.0f), WHITE);
    Renderer::draw(g_stripes, glm::mat4(1.0f), WHITE);
    drawFoundation();
    drawLairPipes(time);
    drawVentAndKeypad(time);
    drawVaultDoor(time);
    drawLights(time);
    drawPorthole(time);
    drawLaneLights(time);
}
