#include "models/PropModels.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "Config.h"
#include <cmath>

void PropModels::init() {}
void PropModels::shutdown() {}

// TODO(D): PLACEHOLDER pizza: crust, cheese, pepperoni, cheese drip (req 104)
void PropModels::drawPizza(const glm::mat4& base, float time)
{
    glm::mat4 m = translate2D(base, 0.0f, sinf(time * 3.0f) * 4.0f);   // bob
    Renderer::draw(Shape::Triangle, part(m, 0.0f, 0.0f, PIZZA_SIZE, PIZZA_SIZE, 180.0f), PIZZA);
    Renderer::draw(Shape::Quad, part(m, 0.0f, PIZZA_SIZE * 0.5f, PIZZA_SIZE * 1.05f, 7.0f), hexColor(0xB5762A));
    Renderer::draw(Shape::Circle, part(m, -6.0f, 6.0f, 9.0f, 9.0f), hexColor(0xB3261E));
    Renderer::draw(Shape::Circle, part(m, 5.0f, -4.0f, 8.0f, 8.0f), hexColor(0xB3261E));
}

// TODO(B): PLACEHOLDER sai: blade, two side prongs, grip (req 75)
void PropModels::drawSai(const glm::mat4& base, float spinDegrees)
{
    glm::mat4 m = rotate2D(base, -spinDegrees);
    Renderer::draw(Shape::Quad, part(m, 6.0f, 0.0f, 26.0f, 4.0f), hexColor(0xC8D0D8));
    Renderer::draw(Shape::Quad, part(m, -10.0f, 0.0f, 10.0f, 6.0f), hexColor(0x5A2E1A));
    Renderer::draw(Shape::Quad, part(m, -4.0f, 0.0f, 3.0f, 16.0f), hexColor(0xC8D0D8));
}

// TODO(B): PLACEHOLDER 4-point star (req 92)
void PropModels::drawShuriken(const glm::mat4& base, float spinDegrees)
{
    glm::mat4 m = rotate2D(base, spinDegrees);
    Renderer::draw(Shape::Quad, part(m, 0.0f, 0.0f, 24.0f, 6.0f), hexColor(0x9AA0A8));
    Renderer::draw(Shape::Quad, part(m, 0.0f, 0.0f, 6.0f, 24.0f), hexColor(0x9AA0A8));
    Renderer::draw(Shape::Circle, part(m, 0.0f, 0.0f, 6.0f, 6.0f), hexColor(0x30343A));
}

// TODO(B): PLACEHOLDER glowing elongated bolt (req 92)
void PropModels::drawBlasterBolt(const glm::mat4& base, float time)
{
    float pulse = 0.8f + 0.2f * sinf(time * 20.0f);
    Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, 34.0f * pulse, 14.0f), withAlpha(hexColor(0xFF5A1F), 0.6f));
    Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, 24.0f, 7.0f), hexColor(0xFFD27A));
}
