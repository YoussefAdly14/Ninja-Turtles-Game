#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>                  
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 
using namespace std;

GLuint VAO, VBO;
float posX = 0;
float posY = 0;
float mouseX = 0;
float mouseY = 0;
bool clicked = false;
bool visible = true;

// --- Shader sources ---
const char* vertexShaderSource = R"(
#version 430 core
layout (location = 0) in vec3 aPos;

uniform mat4 u_Transform;

void main()
{
    gl_Position = u_Transform*vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 430 core
out vec4 FragColor;
uniform float u_Highlight;
void main()
{
    //vec3 finalColor = mix(vec3(1, 0, 0), vec3(1, 1, 0), u_Highlight);
    vec3 finalColor = vec3(1.0f, 0.5f, u_Highlight);
    FragColor = vec4(finalColor, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compile error:\n" << infoLog << std::endl;
    }
    return shader;
}

void createTriangle() {
    // --- Triangle vertex data (NDC coordinates, -1 to 1) ---
    float vertices[] = {
        //first triangle
        -1, -1, 0, // bottom left
        -0.5, -1, 0, // bottom right
        -0.75, -0.5, 0
    };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_W && action == GLFW_REPEAT)
        posY += 0.01;
    else if (key == GLFW_KEY_S && action == GLFW_REPEAT)
        posY -= 0.01;
    else if(key == GLFW_KEY_A && action == GLFW_REPEAT)
        posX -= 0.01;
    else if (key == GLFW_KEY_D && action == GLFW_REPEAT)
        posX += 0.01;

    if (posX <= -1.0f || posX >= 1.0f ||
        posY <= -1.0f || posY >= 1.0f)
    {
        cout << "posx: " << posX << "\nposy: " << posY << endl;
        visible = false;
    }
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) posX -= 0.001;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) posX += 0.001;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) posY += 0.001;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) posY -= 0.001;

    if (posX <= -0.2 || posX >= 1.0f ||
        posY <= -0.2 || posY >= 1.0f)
    {
        visible = false;
    }
       // cout << "posx: " << posX << "\nposy: " << posY << endl;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        clicked = true;

    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        clicked = false;       
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    //cout << "Mouse: " << xpos << ", " << ypos << endl;
    mouseX = (float)xpos / 400.0f - 1.0f;
    mouseY = 1.0f - (float)ypos / 300.0f;
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Triangle", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glViewport(0, 0, 800, 600);

    // --- Build shader program ---
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader link error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    createTriangle();

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    GLint locTransform = glGetUniformLocation(shaderProgram, "u_Transform");
    GLint locHighlight = glGetUniformLocation(shaderProgram, "u_Highlight");
    
    // --- Render loop ---
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (visible) {
            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, glm::vec3(posX, posY, 0.0f));
            glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(transform));
            glDrawArrays(GL_TRIANGLES, 0, 3);
            //glLineWidth(4);
        }

        processInput(window);
        glUniform1f(locHighlight, clicked ? 1.0f : 0.0f);
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);


        glm::mat4 transform1 = glm::mat4(1.0f);
        //center the cursor to be the center of the triangle
        transform1 = glm::translate(transform1, glm::vec3(mouseX + 0.75f, mouseY + 0.8333f, 0.0f));
        glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(transform1));
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Cleanup ---
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}