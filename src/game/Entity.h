#pragma once
#include <glm/glm.hpp>
#include "game/LaneManager.h"

// Fields every turtle, villain, projectile and pizza shares (req 10). Agreed on day 1: change with care.
// Entities live in typed vectors inside World (like Breakout's std::vector<GameObject>), and dead ones
// are removed by World::removeDead() at the end of the frame -- never erase while iterating.
struct Entity
{
    int   id = 0;
    int   lane = 0;           // fixed after creation: characters never change lane (spec 3.1)
    float x = 0.0f;           // world x of the centre of the feet
    float yOffset = 0.0f;     // height above the lane floor (jump arc, projectile band)
    float width = 0.0f;       // hitbox size
    float height = 0.0f;
    int   hp = 1;
    int   maxHp = 1;
    bool  alive = true;
    float age = 0.0f;         // seconds since spawn, drives idle animations

    float left() const   { return x - width * 0.5f; }
    float right() const  { return x + width * 0.5f; }
    float bottom() const { return laneFloorY(lane) + yOffset; }
    float top() const    { return bottom() + height; }

    // 0 at full health -> close to 1 near death. Pass to Renderer::setDamageTint (req 82).
    float damageTint() const { return maxHp > 0 ? 1.0f - static_cast<float>(hp) / maxHp : 0.0f; }

    void takeDamage(int amount)
    {
        hp -= amount;
        if (hp <= 0) { hp = 0; alive = false; }
    }

    // where the health bar goes (req 81, 93)
    glm::vec2 healthBarPosition() const { return glm::vec2(x, top() + 16.0f); }
};

// Same lane and the x ranges overlap (the "contact" rule of spec 3.1).
inline bool overlapsX(const Entity& a, const Entity& b, float extraReach = 0.0f)
{
    return a.lane == b.lane && a.left() - extraReach < b.right() && b.left() < a.right() + extraReach;
}
