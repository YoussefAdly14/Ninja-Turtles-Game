#include "models/Placeholders.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "Config.h"
#include <cmath>

const float PI = 3.14159265f;

// Also a small example of the hierarchy pattern: body -> shoulder joint -> arm -> hand -> weapon.
void drawPlaceholderTurtle(const glm::mat4& base, const Pose& pose, const glm::vec4& maskColor,
                           float weaponLength, const glm::vec4& weaponColor, bool canSwing)
{
    // whole body: squash when ducking, small idle bob
    glm::mat4 body = scale2D(base, 1.0f, pose.ducking ? DUCK_HEIGHT_SCALE : 1.0f);
    body = translate2D(body, 0.0f, sinf(pose.time * 3.0f) * 2.0f);

    // legs swing around the hips while walking
    float legSwing = sinf(pose.walkCycle) * 20.0f;
    Renderer::draw(Shape::Quad, part(joint(body, -12.0f, 40.0f,  legSwing), 0.0f, -20.0f, 16.0f, 40.0f), TURTLE_SKIN);
    Renderer::draw(Shape::Quad, part(joint(body,  12.0f, 40.0f, -legSwing), 0.0f, -20.0f, 16.0f, 40.0f), TURTLE_SKIN);

    // shell, plastron, head, mask
    Renderer::draw(Shape::Circle, part(body, -8.0f, 70.0f, 60.0f, 66.0f), SHELL);
    Renderer::draw(Shape::Circle, part(body,  6.0f, 68.0f, 40.0f, 56.0f), PLASTRON);
    Renderer::draw(Shape::Circle, part(body,  8.0f, 108.0f, 34.0f, 32.0f), TURTLE_SKIN);
    Renderer::draw(Shape::Quad,   part(body,  8.0f, 111.0f, 37.0f, 9.0f), maskColor);

    // arm hangs down from the shoulder; the attack swings it forward and up
    float armAngle = 30.0f + (canSwing ? sinf(pose.attack * PI) * 100.0f : 0.0f);
    glm::mat4 shoulder = joint(body, 10.0f, 88.0f, armAngle);
    Renderer::draw(Shape::Quad, part(shoulder, 0.0f, -15.0f, 12.0f, 30.0f), TURTLE_SKIN);

    // the weapon is a child of the hand, so it moves with the arm ("animates in the hand")
    glm::mat4 hand = translate2D(shoulder, 0.0f, -30.0f);
    Renderer::draw(Shape::Quad, part(hand, weaponLength * 0.5f, 0.0f, weaponLength, 5.0f), weaponColor);
}

// drawn facing +x, then mirrored so it faces left
void drawPlaceholderVillain(const glm::mat4& base, const Pose& pose, float w, float h, const glm::vec4& color)
{
    glm::mat4 body = scale2D(base, -1.0f, 1.0f);

    float legSwing = sinf(pose.walkCycle) * 20.0f;
    Renderer::draw(Shape::Quad, part(joint(body, -w * 0.2f, h * 0.35f,  legSwing), 0.0f, -h * 0.175f, w * 0.25f, h * 0.35f), color);
    Renderer::draw(Shape::Quad, part(joint(body,  w * 0.2f, h * 0.35f, -legSwing), 0.0f, -h * 0.175f, w * 0.25f, h * 0.35f), color);
    Renderer::draw(Shape::Quad, part(body, 0.0f, h * 0.55f, w, h * 0.45f), color);
    Renderer::draw(Shape::Circle, part(body, w * 0.1f, h * 0.88f, w * 0.5f, h * 0.25f), color);

    float armAngle = 30.0f + sinf(pose.attack * PI) * 90.0f;
    glm::mat4 shoulder = joint(body, w * 0.2f, h * 0.72f, armAngle);
    Renderer::draw(Shape::Quad, part(shoulder, 0.0f, -h * 0.13f, w * 0.18f, h * 0.26f), color);
}
