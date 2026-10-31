#include "game/Collectible.h"
#include "core/Transform.h"
#include "models/PropModels.h"
#include "Config.h"

Collectible makePizza(int lane, float x, int value, int id)
{
    Collectible c;
    c.id = id;
    c.lane = lane;
    c.x = x;
    c.width = PIZZA_SIZE;
    c.height = PIZZA_SIZE;
    c.value = value;
    return c;
}

void Collectible::update(float dt)
{
    age += dt;   // drives the bob; no timeout (req 105)
}

void Collectible::draw() const
{
    glm::mat4 base = translate2D(glm::mat4(1.0f), x, bottom() + height * 0.5f);
    PropModels::drawPizza(base, age);
}
