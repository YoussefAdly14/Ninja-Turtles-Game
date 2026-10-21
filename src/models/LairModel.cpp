#include "models/LairModel.h"
#include "core/Renderer.h"
#include "core/Colors.h"
#include "Config.h"
#include <cmath>

void LairModel::init() {}
void LairModel::shutdown() {}

// TODO(D): PLACEHOLDER. Detailed Lair (req 40): vault door with shell emblem, pipes, lights.
void LairModel::draw(float time)
{
    float height = PLAYFIELD_TOP - PLAYFIELD_BOTTOM;
    glm::vec2 center(LAIR_WIDTH * 0.5f, PLAYFIELD_BOTTOM + height * 0.5f);

    Renderer::drawRect(center, { LAIR_WIDTH, height }, hexColor(0x4A3B2A));
    Renderer::drawCircle(center, LAIR_WIDTH * 0.42f, hexColor(0x6B6F73));
    Renderer::drawCircle(center, LAIR_WIDTH * 0.30f, SHELL);
    // mutagen glow pulsing behind the door
    float pulse = 0.5f + 0.5f * sinf(time * 2.0f);
    Renderer::drawCircle(center, LAIR_WIDTH * 0.12f, withAlpha(MUTAGEN_GLOW, 0.5f + 0.4f * pulse));
}
