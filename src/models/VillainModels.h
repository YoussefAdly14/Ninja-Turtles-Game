#pragma once
#include <glm/glm.hpp>
#include "game/Types.h"
#include "models/Pose.h"

// Foot Soldier (owner D, FootModel.cpp, req 85), Bebop + Rocksteady (owner A, VillainModels.cpp, req 87, 89).
// Same contract as TurtleModels.h, except villains face LEFT.
// Each type must read differently by SHAPE, not colour, and show its attack type/difficulty (req 123).
namespace VillainModels
{
    void init();
    void shutdown();
    void initFootSoldier();        // FootModel.cpp meshes, called by init()/shutdown()
    void shutdownFootSoldier();

    void drawFootSoldier(const glm::mat4& base, const Pose& pose);
    void drawBebop(const glm::mat4& base, const Pose& pose);
    void drawRocksteady(const glm::mat4& base, const Pose& pose);
    void draw(VillainKind kind, const glm::mat4& base, const Pose& pose);
}
