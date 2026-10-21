#include "scene/Environment.h"
#include "core/Renderer.h"
#include "core/Colors.h"
#include "Config.h"
#include <cmath>

void Environment::init() {}
void Environment::shutdown() {}

// TODO(D): PLACEHOLDER. Arched brick ceiling, gradient wall, water channel (req 111).
void Environment::drawBackground(float time)
{
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, WORLD_HEIGHT * 0.5f }, { WORLD_WIDTH, WORLD_HEIGHT }, SEWER_WALL);

    // water channel along the bottom with scrolling ripple lines
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, PLAYFIELD_BOTTOM * 0.5f }, { WORLD_WIDTH, PLAYFIELD_BOTTOM }, hexColor(0x24413A));
    float scroll = fmodf(time * 40.0f, 160.0f);
    for (float x = -160.0f; x < WORLD_WIDTH; x += 160.0f)
        Renderer::drawLine({ x + scroll, 25.0f }, { x + scroll + 60.0f, 25.0f }, 3.0f, withAlpha(WHITE, 0.15f));
}

// One static function per element; tick them off in REQUIREMENTS.md as they land.
static void drawPipes(float /*time*/) {}          // TODO(C) req 112: joints, valves, animated drips
static void drawMutagenBarrels(float /*time*/) {} // TODO(D) req 113: bands, hazard symbol, pulsing ooze
static void drawManholes(float /*time*/) {}       // TODO(A) req 114: flat on the walkways
static void drawWorkLamps(float /*time*/) {}      // TODO(A) req 115: cage, bulb, light cone, flicker
static void drawGraffitiWall(float /*time*/) {}   // TODO(C) req 116: original tags + lair sign
static void drawPizzaBoxes(float /*time*/) {}     // TODO(A) req 117 (optional): next to the Lair

void Environment::drawProps(float time)
{
    drawGraffitiWall(time);
    drawPipes(time);
    drawWorkLamps(time);
    drawMutagenBarrels(time);
    drawManholes(time);
    drawPizzaBoxes(time);
}
