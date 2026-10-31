#include "ui/Button.h"
#include "ui/Text.h"
#include "ui/UiStyle.h"
#include "core/Renderer.h"

const float BUTTON_RADIUS = 14.0f;
const float BUTTON_BORDER = 8.0f;
const float BUTTON_LIP = 7.0f;           // how far the dark "3D" edge shows under the button
const float HOVER_GROW = 1.06f;
const float PRESS_SHRINK = 0.97f;
const float LABEL_SCALE = 0.8f;
const float LABEL_DROP = 13.0f;          // baseline below the centre so the caps sit in the middle
const float BUTTON_RIVET_INSET = 18.0f;

bool Button::contains(glm::vec2 p) const
{
    return p.x >= center.x - size.x * 0.5f && p.x <= center.x + size.x * 0.5f &&
           p.y >= center.y - size.y * 0.5f && p.y <= center.y + size.y * 0.5f;
}

// normal: green plate on its lip | hover: lighter, bigger, mutagen glow | pressed: sinks onto the lip
void Button::draw() const
{
    float grow = pressed ? PRESS_SHRINK : (hovered ? HOVER_GROW : 1.0f);
    glm::vec2 s = size * grow;
    glm::vec2 c = center - glm::vec2(0.0f, pressed ? BUTTON_LIP : 0.0f);

    // lip stays where the button rests, the plate sits on top of it
    Renderer::drawRoundedRect(center - glm::vec2(0.0f, BUTTON_LIP), s + glm::vec2(BUTTON_BORDER), BUTTON_RADIUS + 4.0f, UI_FRAME);
    Renderer::drawRoundedRect(center - glm::vec2(0.0f, BUTTON_LIP), s, BUTTON_RADIUS, UI_BUTTON_LIP);

    Renderer::drawRoundedRect(c, s + glm::vec2(BUTTON_BORDER), BUTTON_RADIUS + 4.0f, hovered ? UI_ACCENT : UI_FRAME);
    glm::vec4 fill = pressed ? UI_BUTTON_DOWN : (hovered ? UI_BUTTON_HOVER : UI_BUTTON);
    Renderer::drawRoundedRect(c, s, BUTTON_RADIUS, fill);
    if (!pressed)   // shine along the top edge
        Renderer::drawRect(c + glm::vec2(0.0f, s.y * 0.5f - 8.0f), { s.x - 2.0f * BUTTON_RADIUS - 20.0f, 5.0f }, withAlpha(WHITE, 0.25f));

    UiStyle::drawRivet(c + glm::vec2(-s.x * 0.5f + BUTTON_RIVET_INSET, 0.0f), 5.0f);
    UiStyle::drawRivet(c + glm::vec2( s.x * 0.5f - BUTTON_RIVET_INSET, 0.0f), 5.0f);

    UiStyle::drawOutlinedText(label, c.x, c.y - LABEL_DROP * grow, LABEL_SCALE * grow, UI_TEXT, 2.0f);
}
