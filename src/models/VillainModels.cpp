#include "models/VillainModels.h"
#include "models/Placeholders.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "Config.h"

// Bebop + Rocksteady (owner A). The Foot Soldier lives in FootModel.cpp (owner D).

void VillainModels::init() {}
void VillainModels::shutdown() {}

// TODO(A): PLACEHOLDER. Round bulky warthog: mohawk, tusks, sunglasses, vest, arm blaster (req 87).
void VillainModels::drawBebop(const glm::mat4& base, const Pose& pose)
{
    drawPlaceholderVillain(base, pose, BEBOP_WIDTH, BEBOP_HEIGHT, MUTANT_GRAY);
}

// TODO(A): PLACEHOLDER. Largest villain: rhino horn, helmet, shoulder armour, big fists (req 89).
void VillainModels::drawRocksteady(const glm::mat4& base, const Pose& pose)
{
    drawPlaceholderVillain(base, pose, ROCKSTEADY_WIDTH, ROCKSTEADY_HEIGHT, MUTANT_GRAY);
    // horn, so the placeholder is already told apart from Bebop
    Renderer::draw(Shape::Triangle, part(base, -ROCKSTEADY_WIDTH * 0.35f, ROCKSTEADY_HEIGHT * 0.95f, 18.0f, 30.0f, 40.0f), hexColor(0xE8E0D0));
}

void VillainModels::draw(VillainKind kind, const glm::mat4& base, const Pose& pose)
{
    switch (kind) {
    case VillainKind::FootSoldier: drawFootSoldier(base, pose); break;
    case VillainKind::Bebop:       drawBebop(base, pose);       break;
    case VillainKind::Rocksteady:  drawRocksteady(base, pose);  break;
    default: break;
    }
}
