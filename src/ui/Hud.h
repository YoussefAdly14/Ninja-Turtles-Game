#pragma once
#include <string>
#include <glm/glm.hpp>
#include "game/Types.h"

struct World;

// Top bar: turtle cards (left), hint/tooltip, wave indicator (centre), controls hint, pizza counter (right).
// Plus the big "WAVE 2/3" banner over the playfield at each wave start. Owner C, req 28, 60-66, 103, 119.
// All timers use world.time, so they freeze with the game while paused.
class Hud
{
public:
    int hoveredCard = -1;

    // Returns the kind whose (unlocked) card was clicked this frame with a lane selected, or TurtleKind::Count.
    TurtleKind update(const World& world, glm::vec2 mouseWorld, bool clicked);
    void draw(const World& world) const;

private:
    bool m_cardPressed = false;          // mouse held on the hovered card
    int m_flashCard = -1;                // card that just deployed a turtle flashes white
    float m_flashTime = -100.0f;

    // short hint after an invalid click (req 66, NF15)
    std::string m_hint, m_hintDetail;
    float m_hintTime = -100.0f;

    // pizza counter pulse (req 60)
    int m_lastCoins = -1;
    int m_coinDelta = 0;
    float m_coinTime = -100.0f;

    // wave banner (req 103)
    int m_bannerWave = 0;
    float m_bannerTime = -100.0f;

    void showHint(const World& world, const std::string& text, const std::string& detail);
    void drawCard(int i, const World& world) const;
    void drawHintArea(const World& world) const;
    void drawPizzaCounter(const World& world) const;
    void drawWaveBanner(const World& world) const;
};
