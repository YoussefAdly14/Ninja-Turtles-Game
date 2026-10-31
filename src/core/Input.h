#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// Keyboard + mouse through GLFW callbacks (Lab 3, learnopengl Breakout "Keys[1024]" idea).
// "Pressed"/"Clicked" are true for exactly one frame; "Down" stays true while held.
namespace Input
{
    void install(GLFWwindow* window);   // registers the callbacks
    void endFrame();                    // call once per frame, after update + render

    bool isKeyDown(int key);            // GLFW_KEY_...
    bool wasKeyPressed(int key);
    bool wasMouseClicked();             // left button went down this frame
    bool isMouseDown();                 // left button is held (pressed look on buttons)
    glm::vec2 mouseWorld();             // cursor in world units (letterbox-aware)
}
