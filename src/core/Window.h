#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// GLFW window + OpenGL 3.3 core context + letterboxing (req 1, 4).
// The game always sees a WORLD_WIDTH x WORLD_HEIGHT world; resizing adds black bars instead of stretching.
namespace Window
{
    GLFWwindow* create();     // nullptr on failure
    void destroy();
    GLFWwindow* handle();

    glm::mat4 projection();                       // world units -> clip space (glm::ortho)
    glm::vec2 screenToWorld(double x, double y);  // GLFW cursor position -> world units
}
