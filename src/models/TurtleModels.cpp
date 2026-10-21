#include "models/TurtleModels.h"
#include "models/Placeholders.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "Config.h"

void TurtleModels::init() {}
void TurtleModels::shutdown() {}

// TODO(D): PLACEHOLDERS. Replace with the detailed models from the theme bible (req 69, 72, 76).

void TurtleModels::drawLeonardo(const glm::mat4& base, const Pose& pose)
{
    drawPlaceholderTurtle(base, pose, LEONARDO_MASK, 60.0f, hexColor(0xC8D0D8), true);   // katana
}

void TurtleModels::drawRaphael(const glm::mat4& base, const Pose& pose)
{
    drawPlaceholderTurtle(base, pose, RAPHAEL_MASK, 30.0f, hexColor(0xA0A8B0), true);    // sai
}

void TurtleModels::drawDonatello(const glm::mat4& base, const Pose& pose)
{
    drawPlaceholderTurtle(base, pose, DONATELLO_MASK, 80.0f, hexColor(0x8B5A2B), false); // bo staff
    // manhole-cover shield in front
    Renderer::draw(Shape::Circle, part(base, 32.0f, 60.0f, 22.0f, 70.0f), hexColor(0x555A5E));
}

void TurtleModels::draw(TurtleKind kind, const glm::mat4& base, const Pose& pose)
{
    switch (kind) {
    case TurtleKind::Leonardo:  drawLeonardo(base, pose);  break;
    case TurtleKind::Raphael:   drawRaphael(base, pose);   break;
    case TurtleKind::Donatello: drawDonatello(base, pose); break;
    default: break;
    }
}
