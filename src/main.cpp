// Shell Shock: Defend the Lair -- DMET 502 Computer Graphics, GUC Winter 2026.
// main only creates the window and runs the loop; everything else lives in Game (learnopengl Breakout layout).
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "core/Window.h"
#include "core/Input.h"
#include "core/Renderer.h"
#include "game/Game.h"
#include "Config.h"

int main()
{
    GLFWwindow* window = Window::create();
    if (!window) return -1;
    Input::install(window);

    if (!Renderer::init()) {
        Window::destroy();
        return -1;
    }

    Game game;
    game.init();

    float lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window) && !game.quitRequested)
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (deltaTime > MAX_DELTA_TIME) deltaTime = MAX_DELTA_TIME;   // req 5

        game.update(deltaTime);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);   // also the letterbox bars
        glClear(GL_COLOR_BUFFER_BIT);
        game.render();

        Input::endFrame();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    game.shutdown();
    Renderer::shutdown();
    Window::destroy();
    return 0;
}
