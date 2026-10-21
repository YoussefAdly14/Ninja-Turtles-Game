#include <glad/glad.h>   
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>  

#include <glm/glm.hpp>                  
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>         

int g_screenWidth = 800;
int g_screenHeight = 600;

const char* vertexShaderSource = R"(
#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vColor;

uniform mat4 u_Transform;

void main()
{
    gl_Position = u_Transform * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 430 core

in vec3 vColor;

out vec4 FragColor;

uniform vec4 u_Tint;

void main()
{
    //FragColor = vec4(vColor, 1.0);
    FragColor = vec4(vColor, 1.0) * u_Tint;
}
)";

GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compile error ("
            << (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
            << "):\n" << infoLog << std::endl;
    }
    return shader;
}


int main()
{
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(g_screenWidth, g_screenHeight, "Triangle", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glViewport(0, 0, g_screenWidth, g_screenHeight);

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


    float vertices[] = {
        // positions (NDC)      // colours (RGB)
        -0.5f, -0.5f, 0.0f,     1.0f, 0.0f, 0.0f,   // bottom left  - red
         0.5f, -0.5f, 0.0f,     0.0f, 1.0f, 0.0f,   // bottom right - green
         0.0f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f    // top          - blue
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO); // VAO remembers HOW to read the buffer
    glGenBuffers(1, &VBO);      // VBO is the raw vertex bytes stored on the GPU

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Attribute 0 = position: 3 floats, starting at byte 0 of each vertex,
    // and the next vertex begins 6 floats later (the stride).
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);   // vertex attributes are DISABLED by default

    // Attribute 1 = colour: 3 floats, starting 3 floats (12 bytes) into each vertex, same stride.
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLint locTransform = glGetUniformLocation(shaderProgram, "u_Transform");
    GLint locTint = glGetUniformLocation(shaderProgram, "u_Tint");

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float angle = 0.0f;


    while (!glfwWindowShouldClose(window))
    {

        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float t = currentFrame;
        float offsetX = sin(t) * 0.5f;
        float pulse = 0.75f + 0.25f * sin(t * 2.0f);


        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);


        glm::mat4 transform = glm::mat4(1.0f);

        /*float aspect = (float)g_screenWidth / (float)g_screenHeight;
        transform = glm::scale(transform, glm::vec3(1.0f / aspect, 1.0f, 1.0f));*/

        //transform = glm::translate(transform, glm::vec3(offsetX, 0.0f, 0.0f));
        //transform = glm::rotate(transform, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        //transform = glm::scale(transform, glm::vec3(pulse, pulse, 1.0f));



        //send the matrix to the uniform inside the VERTEX shader
        glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(transform));
        //   locTransform     = the cached location of "u_Transform"
        //   1                = we are sending one matrix
        //   GL_FALSE         = do not transpose it (GLM already stores it
        //                      column-major, which is what OpenGL expects)
        //   glm::value_ptr() = the address of the first of the 16 floats


        float r = sin(t) * 0.5f + 0.5f;
        float g = sin(t + 2.094f) * 0.5f + 0.5f;
        float b = sin(t + 4.188f) * 0.5f + 0.5f;

        //send the matrix to the uniform inside the FRAGMENT shader
        glUniform4f(locTint, r, g, b, 1.0f);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}