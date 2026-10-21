#pragma once
#include "game/Entity.h"
#include "game/Types.h"

// Sai (Raphael), shuriken (Foot Soldier, HIGH), blaster bolt (Bebop, LOW). Req 75, 86, 88, 92.
struct Projectile : Entity
{
    ProjectileKind kind = ProjectileKind::Sai;
    AttackBand band = AttackBand::Mid;
    bool  fromTurtle = true;    // turtle shots never hurt turtles, villain shots never hurt villains
    float velocity = 0.0f;      // units/s, + = right, - = left
    int   damage = 1;
    float spin = 0.0f;          // degrees, for the spinning sai/shuriken

    void update(float dt);
    void draw() const;
};

Projectile makeProjectile(ProjectileKind kind, int lane, float x, bool fromTurtle, int id);
