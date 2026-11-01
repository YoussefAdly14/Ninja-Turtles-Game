#include "core/Input.h"
#include "core/Window.h"

static bool g_keysDown[GLFW_KEY_LAST + 1] = {};
static bool g_keysPressed[GLFW_KEY_LAST + 1] = {};
static bool g_mouseClicked = false;
static bool g_mouseRightClicked = false;
static bool g_mouseDown = false;      // left button held (pressed look on buttons/cards)
static double g_mouseX = 0.0, g_mouseY = 0.0;

static void keyCallback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (key < 0 || key > GLFW_KEY_LAST) return;   // unknown keys come in as -1

    if (action == GLFW_PRESS) {
        g_keysDown[key] = true;
        g_keysPressed[key] = true;
    }
    else if (action == GLFW_RELEASE) {
        g_keysDown[key] = false;
    }
}

static void mouseButtonCallback(GLFWwindow* /*window*/, int button, int action, int /*mods*/)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        g_mouseClicked = true;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        g_mouseRightClicked = true;
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        g_mouseDown = (action == GLFW_PRESS);
}

static void cursorPosCallback(GLFWwindow* /*window*/, double xpos, double ypos)
{
    g_mouseX = xpos;
    g_mouseY = ypos;
}

void Input::install(GLFWwindow* window)
{
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
}

void Input::endFrame()
{
    for (bool& p : g_keysPressed) p = false;
    g_mouseClicked = false;
    g_mouseRightClicked = false;
}

bool Input::isKeyDown(int key)
{
    return key >= 0 && key <= GLFW_KEY_LAST && g_keysDown[key];
}

bool Input::wasKeyPressed(int key)
{
    return key >= 0 && key <= GLFW_KEY_LAST && g_keysPressed[key];
}

bool Input::wasMouseClicked()
{
    return g_mouseClicked;
}

bool Input::wasRightMouseClicked()
{
    return g_mouseRightClicked;
}

bool Input::isMouseDown()
{
    return g_mouseDown;
}

glm::vec2 Input::mouseWorld()
{
    return Window::screenToWorld(g_mouseX, g_mouseY);
}
