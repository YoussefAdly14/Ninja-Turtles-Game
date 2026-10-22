#include "ui/HealthBar.h"
#include "ui/Text.h"
#include "ui/UiStyle.h"
#include "core/Renderer.h"
#include "core/Colors.h"
#include <string>

const glm::vec2 BAR_SIZE(58.0f, 10.0f);
const float FRAME = 3.0f;              // frame thickness around the bar
const float BADGE_SIZE = 28.0f;        // shell disc / Foot diamond that holds the number
const float BADGE_OVERLAP = 6.0f;      // the badge sits over the end of the bar
const float HP_TEXT_SCALE = 0.5f;      // ~17 units tall digits
const float HP_TEXT_DROP = 9.0f;       // baseline below the badge centre so the digits sit in the middle
const glm::vec4 TURTLE_FILL = hexColor(0x3FBF3F);
const glm::vec4 VILLAIN_FILL = hexColor(0xD03030);
const glm::vec4 BAR_EMPTY = hexColor(0x1A1A1A);
const glm::vec4 SEGMENT_LINE = hexColor(0x0C0C0C, 0.8f);

// bar: dark trough, fill grows from the left edge, one tick per hit point so the length reads without colour
static void drawBarFill(glm::vec2 center, int hp, int maxHp, const glm::vec4& fill)
{
    float fraction = maxHp > 0 ? static_cast<float>(hp) / maxHp : 0.0f;
    if (fraction < 0.0f) fraction = 0.0f;
    Renderer::drawRect(center, BAR_SIZE, BAR_EMPTY);
    float fillWidth = BAR_SIZE.x * fraction;
    Renderer::drawRect({ center.x - BAR_SIZE.x * 0.5f + fillWidth * 0.5f, center.y }, { fillWidth, BAR_SIZE.y }, fill);
    Renderer::drawRect({ center.x - BAR_SIZE.x * 0.5f + fillWidth * 0.5f, center.y + BAR_SIZE.y * 0.25f },
                       { fillWidth, 2.0f }, withAlpha(WHITE, 0.3f));   // shine
    for (int i = 1; i < maxHp; ++i) {
        float x = center.x - BAR_SIZE.x * 0.5f + BAR_SIZE.x * i / maxHp;
        Renderer::drawRect({ x, center.y }, { 1.5f, BAR_SIZE.y }, SEGMENT_LINE);
    }
}

// turtles: rounded shell-brown frame + a little shell disc on the LEFT end with the number
static void drawTurtleBar(glm::vec2 center, int hp, int maxHp, const std::string& number)
{
    glm::vec2 frame = BAR_SIZE + glm::vec2(2.0f * FRAME);
    Renderer::drawRoundedRect(center, frame + glm::vec2(2.0f), (frame.y + 2.0f) * 0.5f, BLACK);
    Renderer::drawRoundedRect(center, frame, frame.y * 0.5f, SHELL);
    drawBarFill(center, hp, maxHp, TURTLE_FILL);

    // shell badge: dark rim, shell, plastron-coloured ring of scutes, number on top
    glm::vec2 badge(center.x - BAR_SIZE.x * 0.5f - BADGE_SIZE * 0.5f + BADGE_OVERLAP, center.y);
    Renderer::drawCircle(badge, BADGE_SIZE * 0.5f + 2.0f, BLACK);
    Renderer::drawCircle(badge, BADGE_SIZE * 0.5f, PLASTRON);
    Renderer::drawCircle(badge, BADGE_SIZE * 0.5f - 3.0f, SHELL);
    UiStyle::drawOutlinedText(number, badge.x, badge.y - HP_TEXT_DROP, HP_TEXT_SCALE, WHITE, 1.5f);
}

// villains: angular black frame with pointed ends + a Foot-style diamond on the RIGHT end with the number
static void drawVillainBar(glm::vec2 center, int hp, int maxHp, const std::string& number)
{
    glm::vec2 frame = BAR_SIZE + glm::vec2(2.0f * FRAME);
    float point = frame.y * 0.72f;   // side of the rotated square that makes each pointed end
    Renderer::drawRect({ center.x - frame.x * 0.5f, center.y }, { point, point }, BLACK, 45.0f);
    Renderer::drawRect({ center.x + frame.x * 0.5f, center.y }, { point, point }, BLACK, 45.0f);
    Renderer::drawRect(center, frame, BLACK);
    drawBarFill(center, hp, maxHp, VILLAIN_FILL);

    // diamond badge: red edge, Foot Clan purple-black inside
    glm::vec2 badge(center.x + BAR_SIZE.x * 0.5f + BADGE_SIZE * 0.5f - BADGE_OVERLAP, center.y);
    float side = BADGE_SIZE * 0.72f;
    Renderer::drawRect(badge, glm::vec2(side + 4.0f), BLACK, 45.0f);
    Renderer::drawRect(badge, glm::vec2(side), VILLAIN_FILL, 45.0f);
    Renderer::drawRect(badge, glm::vec2(side - 5.0f), FOOT_CLAN, 45.0f);
    UiStyle::drawOutlinedText(number, badge.x, badge.y - HP_TEXT_DROP, HP_TEXT_SCALE, WHITE, 1.5f);
}

// Bar length + segment ticks + number: readable without colour (req 81, 93, NF19). The number sits on the
// OUTER side (turtles face right, villains left), so in a fight the two badges never land on each other's bar.
void drawHealthBar(glm::vec2 center, int hp, int maxHp, bool isTurtle)
{
    std::string number = std::to_string(hp > 0 ? hp : 0);
    if (isTurtle) drawTurtleBar(center, hp, maxHp, number);
    else drawVillainBar(center, hp, maxHp, number);
}
