#pragma once
#include <string>
#include <glm/glm.hpp>
#include "core/Colors.h"

// One look for every menu, button, card and HUD plate (req 18, NF16):
// one font (Bangers, ui/Text), one palette (below, built on the theme-bible colours), these few helpers.
const glm::vec4 UI_FRAME        = hexColor(0x0E1311);   // outlines around plates and buttons
const glm::vec4 UI_PANEL        = hexColor(0x26322E);   // riveted sewer-metal plate
const glm::vec4 UI_PANEL_LIGHT  = hexColor(0x3C4C46);   // top bevel of a plate
const glm::vec4 UI_PANEL_DARK   = hexColor(0x18201D);   // insets (card portrait, volume meter)
const glm::vec4 UI_RIVET        = hexColor(0x77877F);
const glm::vec4 UI_HUD_BAR      = hexColor(0x1E2624);
const glm::vec4 UI_PIPE         = hexColor(0x56655E);
const glm::vec4 UI_TEXT         = WHITE;
const glm::vec4 UI_TEXT_DIM     = hexColor(0xA8A8A8);   // secondary text + locked-card text (grey, req 62)
const glm::vec4 UI_ACCENT       = MUTAGEN_GLOW;         // hover glow, key names, good news
const glm::vec4 UI_WARNING      = RAPHAEL_MASK;         // hints, lose screen
const glm::vec4 UI_BUTTON       = TURTLE_SKIN;
const glm::vec4 UI_BUTTON_HOVER = hexColor(0x5FB436);
const glm::vec4 UI_BUTTON_DOWN  = hexColor(0x356E1C);
const glm::vec4 UI_BUTTON_LIP   = hexColor(0x1F3F15);   // the dark "3D" edge under a button

namespace UiStyle
{
    // riveted metal plate with a dark frame (menus, HUD plates, tooltips)
    void drawPanel(glm::vec2 center, glm::vec2 size);
    void drawRivet(glm::vec2 center, float radius);

    // text with a black outline + drop shadow so it reads on any background; centred on centerX
    void drawOutlinedText(const std::string& text, float centerX, float baselineY, float scale,
                          const glm::vec4& color, float outline);

    // largest scale <= maxScale at which text fits in maxWidth
    float fitScale(const std::string& text, float maxScale, float maxWidth);

    // small padlock (locked cards), size = total height
    void drawPadlock(glm::vec2 center, float size);
}
