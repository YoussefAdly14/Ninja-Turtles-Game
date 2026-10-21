#pragma once
#include <glm/glm.hpp>

// Remaining health, always visible above every character (owner C, req 81, 93, NF19).
// Bar length + number, so it reads without colour. Turtles and villains get different frames.
void drawHealthBar(glm::vec2 center, int hp, int maxHp, bool isTurtle);
