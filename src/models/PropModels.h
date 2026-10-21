#pragma once
#include <glm/glm.hpp>

// Small objects (owner D for pizza, B for projectiles: req 75, 92, 104).
// base = world matrix of the object's CENTRE. Model units = world units.
namespace PropModels
{
    void init();
    void shutdown();

    void drawPizza(const glm::mat4& base, float time);           // collectible + HUD coin icon, ~PIZZA_SIZE
    void drawSai(const glm::mat4& base, float spinDegrees);      // Raphael's thrown sai
    void drawShuriken(const glm::mat4& base, float spinDegrees); // Foot Soldier, HIGH
    void drawBlasterBolt(const glm::mat4& base, float time);     // Bebop, LOW
}
