#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Parent -> child transforms for composing models from parts (req 7).
// Sources: learnopengl Getting-started/Transformations, Guest-Articles/2021/Scene/Scene-Graph,
// Lab 4 orbiting cube (translate to the pivot, rotate, then place the child).
//
// A part's matrix = parent matrix * its local matrix, so moving or rotating the
// parent (torso) moves every child (arm -> hand -> katana) with it.

inline glm::mat4 translate2D(const glm::mat4& m, float x, float y)
{
    return glm::translate(m, glm::vec3(x, y, 0.0f));
}

inline glm::mat4 rotate2D(const glm::mat4& m, float degrees)
{
    return glm::rotate(m, glm::radians(degrees), glm::vec3(0.0f, 0.0f, 1.0f));
}

inline glm::mat4 scale2D(const glm::mat4& m, float sx, float sy)
{
    return glm::scale(m, glm::vec3(sx, sy, 1.0f));
}

// A joint (shoulder, elbow, hip...): move to the pivot inside the parent, then rotate.
// Everything drawn relative to the result swings around that pivot.
inline glm::mat4 joint(const glm::mat4& parent, float pivotX, float pivotY, float degrees)
{
    return rotate2D(translate2D(parent, pivotX, pivotY), degrees);
}

// Final matrix for one visible part: centre (x, y) in its parent, size w x h, optional tilt.
// Don't use the result as a parent for other parts: the scale would stretch them too.
inline glm::mat4 part(const glm::mat4& parent, float x, float y, float w, float h, float degrees = 0.0f)
{
    return scale2D(rotate2D(translate2D(parent, x, y), degrees), w, h);
}
