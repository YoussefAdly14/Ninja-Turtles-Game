// Shell Shock: Defend the Lair -- DMET 502 Computer Graphics, GUC Winter 2026.
// main only creates the window and runs the loop; everything else lives in Game (learnopengl Breakout layout).
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include "core/Window.h"
#include "core/Input.h"
#include "core/Renderer.h"
#include "game/Game.h"
#include "dev/Gallery.h"
#include "Config.h"

int main(int argc, char** argv)
{
    // dev aids, never needed to play:
    //   --gallery turtles|villains|props   every model in every pose
    //   --demo                             skip menu + cutscene, turtles already placed, fixed seed
    //   --cutscene | --sound               open that screen directly
    //   --pause | --win | --lose           that menu over a demo game that already ran 12 s
    int galleryPage = -1;
    bool demo = false;
    std::string devScreen;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--gallery" && i + 1 < argc) galleryPage = Gallery::pageFromName(argv[++i]);
        else if (arg == "--demo") demo = true;
        else if (arg == "--cutscene" || arg == "--sound" || arg == "--pause" || arg == "--win" || arg == "--lose")
            devScreen = arg.substr(2);
    }

    GLFWwindow* window = Window::create();
    if (!window) return -1;
    Input::install(window);

    if (!Renderer::init()) {
        Window::destroy();
        return -1;
    }

    Game game;
    game.init();
    if (demo) game.startDemo();
    if (!devScreen.empty()) game.devStart(devScreen);

    float lastFrame = static_cast<float>(glfwGetTime());
    float galleryTime = 0.0f;

    while (!glfwWindowShouldClose(window) && !game.quitRequested)
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (deltaTime > MAX_DELTA_TIME) deltaTime = MAX_DELTA_TIME;   // req 5

        if (galleryPage >= 0) galleryTime += deltaTime;
        else game.update(deltaTime);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);   // also the letterbox bars
        glClear(GL_COLOR_BUFFER_BIT);
        if (galleryPage >= 0) {
            Renderer::beginFrame(Window::projection());
            Gallery::draw(galleryPage, galleryTime);
        }
        else {
            game.render();
        }

        Input::endFrame();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    game.shutdown();
    Renderer::shutdown();
    Window::destroy();
    return 0;
}
