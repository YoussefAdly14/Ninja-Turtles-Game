#pragma once
#include "game/Entity.h"

// Pizza slice dropped by a defeated villain; a turtle walking into it collects it (req 104-106, OQ13).
struct Collectible : Entity
{
    int value = 0;

    void update(float dt);
    void draw() const;
};

Collectible makePizza(int lane, float x, int value, int id);
