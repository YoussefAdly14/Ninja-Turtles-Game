#include "models/VillainModels.h"
#include "models/Placeholders.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "Config.h"

// Foot Soldier (owner D). Declared in VillainModels.h with the other villains.

// TODO(D): PLACEHOLDER. Lean angular ninja: hood, eye-slit mask, Foot emblem, armour plates,
// belt, shuriken pouch; overhead throwing pose (req 85, 91).
void VillainModels::drawFootSoldier(const glm::mat4& base, const Pose& pose)
{
    drawPlaceholderVillain(base, pose, FOOT_WIDTH, FOOT_HEIGHT, FOOT_CLAN);
}
