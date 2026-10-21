#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Global VBOs, VAOs AND EBO
GLuint VAO[2];
GLuint EBO;
GLuint VBO[2];

const int TRIANGLE_VERTEX_COUNT = 6; // vertices 0 -> 5 belong to the triangle
int circleVertexStart = 0; // filled in once the circle data is appended for flexibility
int circleVertexCount = 0; // Same here     
bool isMousePressed = false; //Checking if we pressed the mouse button, updates every frame

// Le shaders :3

const char* vertexShaderSource = R"(
#version 430 core

layout (location = 0) in vec3 aPos; //You've got positions
layout (location = 1) in vec3 aColor; //You've got colors!

out vec3 vColor;

uniform mat4 u_Transform; //And your transformations, wooo!

void main()
{
    gl_Position = u_Transform * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 430 core

in vec3 vColor;
out vec4 FragColor; //We've got colours coming in and colors coming out!

uniform float u_Highlight; //Whether or not we want it yellow! 0 for no, 1 for yes

void main()
{
    vec3 finalColor = mix(vColor, vec3(1.0, 1.0, 0.0), u_Highlight); //Calculating the color we want at last, uses interpolation (WOW)
    FragColor = vec4(finalColor, 1.0);
}
)";

//The amazing shader compilation code <3
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

// Triangle movement
float posX = 0.0f, posY = 0.0f;
const float moveSpeed = 0.02f;
const float triangleScale = 0.5f; //This is to ensure that everything is consistent for when we collision detect

//Optional keyboard
/* void processInput(GLFWwindow window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) posX -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) posX += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) posY += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) posY -= moveSpeed;
}
*/

//Keyboard input!
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //GLFW_PRESS: Once per click, you need to reclick to do (do for things you don't want spammed)
    //GLFW_REPEAT: As long as you're pressing (good for videogame movement logic or scrolling)
    //GLFW_RELEASE: On key up, once it releases, good for reset logic


    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_A && action == GLFW_REPEAT) posX -= moveSpeed;
    if (key == GLFW_KEY_D && action == GLFW_REPEAT) posX += moveSpeed;
    if (key == GLFW_KEY_W && action == GLFW_REPEAT) posY += moveSpeed;
    if (key == GLFW_KEY_S && action == GLFW_REPEAT) posY -= moveSpeed;
}


//Mouse logic for the clickies
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT) //The click, not the right click
        return;

    if (action == GLFW_PRESS)
        isMousePressed = true;
    else if (action == GLFW_RELEASE) //Here is release in action and the reset logic WEEE
        isMousePressed = false;
}

// Determining where the point is through Cross Product
float triSign(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3)
{
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}
//Detecting if we are inside triangle
bool pointInTriangle(glm::vec2 pt, glm::vec2 a, glm::vec2 b, glm::vec2 c)
{
    float d1 = triSign(pt, a, b);
    float d2 = triSign(pt, b, c);
    float d3 = triSign(pt, c, a);

    bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(hasNeg && hasPos);
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int screenW = 800, screenH = 600;
    GLFWwindow* window = glfwCreateWindow(screenW, screenH, "Triangle", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glViewport(0, 0, screenW, screenH);
    glfwSwapInterval(1);

    // Build shader program
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
    //It's a verctor now so we can stuff to it :/ (if we're using static shapes can leave as a list and just add here)
    std::vector<float> vertices = {
        //Positions             //Colors
        -0.5f, -0.5f, 0.0f,     1.0f, 0.0f, 0.0f, //0
        -0.25f, 0.0f, 0.0f,     0.5f, 0.0f, 0.5f, //1
         0.0f, -0.5f, 0.0f,     0.5f, 0.5f, 0.0f, //2
         0.5f, -0.5f, 0.0f,     0.0f, 1.0f, 0.0f, //3
         0.25f, 0.0f, 0.0f,     0.0f, 0.5f, 0.5f, //4
         0.0f, 0.5f, 0.0f,      0.0f, 0.0f, 1.0f  //5
    };

    GLuint indices[] = {
        0, 2, 1,
        2, 3, 4,
        4, 5, 1
    };

    circleVertexStart = TRIANGLE_VERTEX_COUNT; // = 6
    const int segments = 50; //increase to make smoother looking circle
    std::vector<float> vertCir = {};
    vertCir.insert(vertCir.end(), { 0.0f, 0.0f, 0.0f,  0.0f, 0.8f, 1.0f }); // center and cyan color
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14f * (float)i / (float)segments;
        vertCir.insert(vertCir.end(), {
            cosf(angle), sinf(angle), 0.0f,
            0.0f, 0.8f, 1.0f
            });
    }
    circleVertexCount = segments + 2; // center + segments and because triangle fan... +1 for end :3

    glGenVertexArrays(2, VAO);
    glGenBuffers(2, VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO[0]);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    //EBO is only for the small triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBufferData(GL_ARRAY_BUFFER, vertCir.size() * sizeof(float), vertCir.data(), GL_STATIC_DRAW);




    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GLint locTransform = glGetUniformLocation(shaderProgram, "u_Transform");
    GLint locHighlight = glGetUniformLocation(shaderProgram, "u_Highlight");

    //Saving OG triangle vertices for later manipulation
    glm::vec2 cornerA(-0.5f, -0.5f); // vertex 0
    glm::vec2 cornerB(0.5f, -0.5f);  // vertex 3
    glm::vec2 cornerC(0.0f, 0.5f);   // vertex 5

    while (!glfwWindowShouldClose(window)) {
        //Optional keyboard
        //processInput(window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //Triangle transformation
        glm::mat4 triangleTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.25 + posX, posY, 0.0f));
        triangleTransform = glm::scale(triangleTransform, glm::vec3(triangleScale, triangleScale, 1.0f));

        // The collision test
        glm::vec2 a = glm::vec2(triangleTransform * glm::vec4(cornerA, 0.0f, 1.0f));
        glm::vec2 b = glm::vec2(triangleTransform * glm::vec4(cornerB, 0.0f, 1.0f));
        glm::vec2 c = glm::vec2(triangleTransform * glm::vec4(cornerC, 0.0f, 1.0f));

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        glm::vec2 cursorNDC(
            (2.0f * (float)mouseX) / screenW - 1.0f,
            1.0f - (2.0f * (float)mouseY) / screenH // GLFW's y-axis is pointing upwards; Computers's y-axis is pointing downwards
        );

        bool hovering = pointInTriangle(cursorNDC, a, b, c);
        bool triangleHighlight = hovering;      // hovering the triangle highlights the triangle
        bool circleHighlight = isMousePressed;  // any press (anywhere) highlights the circle

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO[0]);

        // draw the triangle: indexed, uses the EBO, 9 indices for the three small triangles
        glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(triangleTransform));
        glUniform1f(locHighlight, triangleHighlight ? 1.0f : 0.0f);
        glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0);

        // draw the circle: same VBO, different vertex range
        glBindVertexArray(VAO[1]);
        glm::mat4 circleTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.6f, 0.0f, 0.0f));
        circleTransform = glm::scale(circleTransform, glm::vec3(0.25f, 0.25f, 1.0f));
        glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(circleTransform));
        glUniform1f(locHighlight, circleHighlight ? 1.0f : 0.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(2, VAO);
    glDeleteBuffers(2, VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}