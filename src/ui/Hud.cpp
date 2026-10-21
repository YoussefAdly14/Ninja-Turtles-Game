#include "ui/Hud.h"
#include "ui/Text.h"
#include "game/World.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "models/TurtleModels.h"
#include "models/PropModels.h"
#include "Config.h"
#include <string>

const glm::vec2 CARD_SIZE(110.0f, 120.0f);
const float CARD_GAP = 15.0f;
const float CARDS_LEFT = 25.0f;
const float PORTRAIT_SCALE = 0.62f;

static glm::vec2 cardCenter(int i)
{
    return glm::vec2(CARDS_LEFT + CARD_SIZE.x * 0.5f + i * (CARD_SIZE.x + CARD_GAP),
                     WORLD_HEIGHT - HUD_HEIGHT * 0.5f);
}

static bool insideCard(int i, glm::vec2 p)
{
    glm::vec2 c = cardCenter(i);
    return p.x >= c.x - CARD_SIZE.x * 0.5f && p.x <= c.x + CARD_SIZE.x * 0.5f &&
           p.y >= c.y - CARD_SIZE.y * 0.5f && p.y <= c.y + CARD_SIZE.y * 0.5f;
}

TurtleKind Hud::update(const World& world, glm::vec2 mouseWorld, bool clicked)
{
    hoveredCard = -1;
    for (int i = 0; i < (int)TurtleKind::Count; ++i) {
        // locked cards ignore hover and clicks (req 62)
        if (insideCard(i, mouseWorld) && world.economy.isUnlocked((TurtleKind)i))
            hoveredCard = i;
    }
    if (clicked && hoveredCard >= 0) return (TurtleKind)hoveredCard;
    return TurtleKind::Count;
}

// TODO(C): final card art, name + cost text, coin pulse, wave banner (req 60, 61, 103, 119)
void Hud::draw(const World& world) const
{
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, WORLD_HEIGHT - HUD_HEIGHT * 0.5f }, { WORLD_WIDTH, HUD_HEIGHT }, hexColor(0x1E2624));

    for (int i = 0; i < (int)TurtleKind::Count; ++i) {
        TurtleKind kind = (TurtleKind)i;
        glm::vec2 c = cardCenter(i);
        bool unlocked = world.economy.isUnlocked(kind);

        // locked = grayscale through the shader, not a grey box on top (req 62)
        Renderer::setGrayscale(unlocked ? 0.0f : 1.0f);
        Renderer::drawRect(c, CARD_SIZE + glm::vec2(6.0f), i == hoveredCard ? MUTAGEN_GLOW : BLACK);
        Renderer::drawRect(c, CARD_SIZE, hexColor(0x5A4A3A));

        // portrait = the real model, scaled down (req 61)
        glm::mat4 base = translate2D(glm::mat4(1.0f), c.x - 8.0f, c.y - CARD_SIZE.y * 0.5f + 22.0f);
        TurtleModels::draw(kind, scale2D(base, PORTRAIT_SCALE, PORTRAIT_SCALE), Pose());

        Text::drawCentered(std::to_string(turtleCost(kind)), c.x, c.y - CARD_SIZE.y * 0.5f + 4.0f, 0.5f, WHITE);
        Renderer::setGrayscale(0.0f);
    }

    Text::drawCentered("Wave " + std::to_string(world.waves.waveNumber()) + "/" + std::to_string(WAVE_COUNT),
                       WORLD_WIDTH * 0.5f, WORLD_HEIGHT - HUD_HEIGHT * 0.5f, 1.0f, WHITE);

    // pizza counter, top-right
    glm::mat4 icon = translate2D(glm::mat4(1.0f), WORLD_WIDTH - 170.0f, WORLD_HEIGHT - HUD_HEIGHT * 0.5f);
    PropModels::drawPizza(scale2D(icon, 1.4f, 1.4f), 0.0f);
    Text::draw(std::to_string(world.economy.coins), WORLD_WIDTH - 130.0f, WORLD_HEIGHT - HUD_HEIGHT * 0.5f - 15.0f, 1.0f, PIZZA);
}
