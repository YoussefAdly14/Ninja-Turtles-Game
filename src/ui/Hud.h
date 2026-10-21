#pragma once
#include <glm/glm.hpp>
#include "game/Types.h"

struct World;

// Top bar: turtle cards (left), wave indicator (centre), pizza counter (right). Owner C, req 60-66, 119.
class Hud
{
public:
    int hoveredCard = -1;

    // Returns the kind whose (unlocked) card was clicked this frame, or TurtleKind::Count.
    TurtleKind update(const World& world, glm::vec2 mouseWorld, bool clicked);
    void draw(const World& world) const;
};
