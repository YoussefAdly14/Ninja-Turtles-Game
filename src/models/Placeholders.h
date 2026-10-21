#pragma once
#include <glm/glm.hpp>
#include "models/Pose.h"

// TEMPORARY stand-in characters so gameplay can be built before the real models exist
// (working rule 2: "build gameplay with placeholder boxes first").
// Delete this file (and Placeholders.cpp) once every real model is in.
void drawPlaceholderTurtle(const glm::mat4& base, const Pose& pose, const glm::vec4& maskColor,
                           float weaponLength, const glm::vec4& weaponColor, bool canSwing);
void drawPlaceholderVillain(const glm::mat4& base, const Pose& pose, float width, float height,
                            const glm::vec4& color);
