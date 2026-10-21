#include "ui/HealthBar.h"
#include "ui/Text.h"
#include "core/Renderer.h"
#include "core/Colors.h"
#include <string>

const glm::vec2 BAR_SIZE(60.0f, 9.0f);

// TODO(C): shell frame for turtles, Foot frame for villains (req 81)
void drawHealthBar(glm::vec2 center, int hp, int maxHp, bool isTurtle)
{
    float fraction = maxHp > 0 ? static_cast<float>(hp) / maxHp : 0.0f;
    glm::vec4 fill = isTurtle ? hexColor(0x3FBF3F) : hexColor(0xD03030);

    Renderer::drawRect(center, BAR_SIZE + glm::vec2(4.0f), BLACK);
    Renderer::drawRect(center, BAR_SIZE, hexColor(0x333333));
    // fill grows from the left edge
    float fillWidth = BAR_SIZE.x * fraction;
    Renderer::drawRect({ center.x - BAR_SIZE.x * 0.5f + fillWidth * 0.5f, center.y }, { fillWidth, BAR_SIZE.y }, fill);

    Text::drawCentered(std::to_string(hp), center.x, center.y + 10.0f, 0.4f, WHITE);
}
