#include "ui/Button.h"
#include "ui/Text.h"
#include "core/Renderer.h"
#include "core/Colors.h"

bool Button::contains(glm::vec2 p) const
{
    return p.x >= center.x - size.x * 0.5f && p.x <= center.x + size.x * 0.5f &&
           p.y >= center.y - size.y * 0.5f && p.y <= center.y + size.y * 0.5f;
}

// TODO(C): themed look (req 18, 19); pressed state
void Button::draw() const
{
    float grow = hovered ? 1.06f : 1.0f;
    Renderer::drawRect(center, size * grow + glm::vec2(8.0f), BLACK);
    Renderer::drawRect(center, size * grow, hovered ? MUTAGEN_GLOW : TURTLE_SKIN);
    Text::drawCentered(label, center.x, center.y - 12.0f, 0.8f, hovered ? BLACK : WHITE);
}
