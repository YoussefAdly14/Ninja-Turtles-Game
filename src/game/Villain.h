#pragma once
#include "game/Entity.h"
#include "game/Types.h"
#include "models/Pose.h"

struct World;

// Foot Soldier (easy, HIGH shuriken), Bebop (medium, LOW blaster), Rocksteady (hard, fast melee). Spec 3.3.
struct Villain : Entity
{
    VillainKind kind = VillainKind::FootSoldier;
    float speed = 0.0f;
    float attackTimer = 0.0f;
    float attackAnim = 0.0f;
    bool  fighting = false;     // touching a turtle: stand still (set by Combat each frame)
    float walkCycle = 0.0f;
    float hitFlash = 0.0f;

    void update(float dt, World& world);
    void draw() const;
    Pose pose() const;
};

Villain makeVillain(VillainKind kind, int lane, int id);
const char* villainName(VillainKind kind);
