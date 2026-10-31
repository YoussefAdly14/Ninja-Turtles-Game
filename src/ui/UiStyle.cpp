#include "ui/UiStyle.h"
#include "ui/Text.h"
#include "core/Renderer.h"
#include "core/Transform.h"

const float PANEL_BORDER = 10.0f;
const float PANEL_RADIUS = 14.0f;
const float PANEL_BEVEL = 6.0f;
const float RIVET_INSET = 16.0f;
const float RIVET_RADIUS = 5.0f;

void UiStyle::drawRivet(glm::vec2 center, float radius)
{
    Renderer::drawCircle(center, radius, UI_FRAME);
    Renderer::drawCircle(center + glm::vec2(-radius * 0.15f, radius * 0.15f), radius * 0.7f, UI_RIVET);
}

void UiStyle::drawPanel(glm::vec2 center, glm::vec2 size)
{
    Renderer::drawRoundedRect(center, size + glm::vec2(PANEL_BORDER), PANEL_RADIUS + PANEL_BORDER * 0.5f, UI_FRAME);
    Renderer::drawRoundedRect(center, size, PANEL_RADIUS, UI_PANEL);
    // lighter strip along the top edge = bevel
    Renderer::drawRect(center + glm::vec2(0.0f, size.y * 0.5f - PANEL_BEVEL), { size.x - 2.0f * PANEL_RADIUS, PANEL_BEVEL }, UI_PANEL_LIGHT);

    glm::vec2 half = size * 0.5f - glm::vec2(RIVET_INSET);
    drawRivet(center + glm::vec2(-half.x,  half.y), RIVET_RADIUS);
    drawRivet(center + glm::vec2( half.x,  half.y), RIVET_RADIUS);
    drawRivet(center + glm::vec2(-half.x, -half.y), RIVET_RADIUS);
    drawRivet(center + glm::vec2( half.x, -half.y), RIVET_RADIUS);
}

// outline = the same text drawn 8 times around the spot in black, then the colour on top
void UiStyle::drawOutlinedText(const std::string& text, float centerX, float baselineY, float scale,
                               const glm::vec4& color, float outline)
{
    glm::vec4 shadow = withAlpha(BLACK, color.a);
    Text::drawCentered(text, centerX + outline * 0.6f, baselineY - outline * 1.6f, scale, shadow);   // drop shadow
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx != 0 || dy != 0)
                Text::drawCentered(text, centerX + dx * outline, baselineY + dy * outline, scale, shadow);
    Text::drawCentered(text, centerX, baselineY, scale, color);
}

float UiStyle::fitScale(const std::string& text, float maxScale, float maxWidth)
{
    float w = Text::width(text, maxScale);
    if (w <= maxWidth || w <= 0.0f) return maxScale;
    return maxScale * maxWidth / w;
}

// shackle = a ring, the body covers its lower half
void UiStyle::drawPadlock(glm::vec2 center, float size)
{
    glm::vec2 bodySize(size * 0.8f, size * 0.55f);
    glm::vec2 bodyCenter = center - glm::vec2(0.0f, size * 0.5f - bodySize.y * 0.5f);
    float shackle = size * 0.62f;
    glm::vec2 shackleCenter = bodyCenter + glm::vec2(0.0f, bodySize.y * 0.5f);

    glm::mat4 at = translate2D(glm::mat4(1.0f), shackleCenter.x, shackleCenter.y);
    Renderer::draw(Shape::Ring, scale2D(at, shackle + 4.0f, shackle + 4.0f), UI_FRAME);
    Renderer::draw(Shape::Ring, scale2D(at, shackle, shackle), hexColor(0xB8B8B8));
    Renderer::drawRoundedRect(bodyCenter, bodySize + glm::vec2(4.0f), 5.0f, UI_FRAME);
    Renderer::drawRoundedRect(bodyCenter, bodySize, 4.0f, hexColor(0xD9A931));
    // keyhole
    Renderer::drawCircle(bodyCenter + glm::vec2(0.0f, size * 0.05f), size * 0.07f, UI_FRAME);
    Renderer::drawRect(bodyCenter - glm::vec2(0.0f, size * 0.05f), { size * 0.06f, size * 0.16f }, UI_FRAME);
}
