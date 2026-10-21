#include "core/Window.h"
#include "Config.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

static GLFWwindow* g_window = nullptr;

// framebuffer size and the letterboxed viewport inside it (pixels)
static int g_fbWidth = 0, g_fbHeight = 0;
static int g_vpX = 0, g_vpY = 0, g_vpWidth = 0, g_vpHeight = 0;

static void updateViewport(int width, int height)
{
    if (width <= 0 || height <= 0) return;   // minimised: keep the old viewport

    float scale = std::min(width / WORLD_WIDTH, height / WORLD_HEIGHT);
    g_fbWidth = width;
    g_fbHeight = height;
    g_vpWidth = static_cast<int>(WORLD_WIDTH * scale);
    g_vpHeight = static_cast<int>(WORLD_HEIGHT * scale);
    g_vpX = (width - g_vpWidth) / 2;
    g_vpY = (height - g_vpHeight) / 2;
    glViewport(g_vpX, g_vpY, g_vpWidth, g_vpHeight);
}

static void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
{
    updateViewport(width, height);
}

GLFWwindow* Window::create()
{
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    g_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (!g_window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1);   // v-sync (req 5)
    glfwSetFramebufferSizeCallback(g_window, framebufferSizeCallback);

    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::cerr << "Failed to initialize OpenGL context\n";
        glfwTerminate();
        return nullptr;
    }
    std::cout << "OpenGL " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << " loaded\n";

    int width, height;
    glfwGetFramebufferSize(g_window, &width, &height);
    updateViewport(width, height);
    return g_window;
}

void Window::destroy()
{
    glfwTerminate();
    g_window = nullptr;
}

GLFWwindow* Window::handle()
{
    return g_window;
}

glm::mat4 Window::projection()
{
    return glm::ortho(0.0f, WORLD_WIDTH, 0.0f, WORLD_HEIGHT, -1.0f, 1.0f);
}

// Lab 3 screen -> NDC idea, but into world units and through the letterbox viewport
glm::vec2 Window::screenToWorld(double x, double y)
{
    int winWidth, winHeight;
    glfwGetWindowSize(g_window, &winWidth, &winHeight);
    if (winWidth <= 0 || winHeight <= 0 || g_vpWidth <= 0) return glm::vec2(-1.0f);

    // window coordinates -> framebuffer pixels (they differ on high-DPI screens)
    float px = static_cast<float>(x) * g_fbWidth / winWidth;
    float py = static_cast<float>(y) * g_fbHeight / winHeight;
    py = g_fbHeight - py;   // screen y points down, world y points up

    return glm::vec2((px - g_vpX) / g_vpWidth * WORLD_WIDTH,
                     (py - g_vpY) / g_vpHeight * WORLD_HEIGHT);
}
