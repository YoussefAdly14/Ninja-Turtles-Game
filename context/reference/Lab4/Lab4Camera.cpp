#include <glad/glad.h>      
#include <GLFW/glfw3.h>     
#include <iostream>
#include <iomanip>          
#include <vector>           
#include <cmath>            
#include <cstdio>           

#include <glm/glm.hpp>                   
#include <glm/gtc/matrix_transform.hpp>  
#include <glm/gtc/type_ptr.hpp>          

int g_screenWidth = 1280;
int g_screenHeight = 720;

const glm::vec3 CAMERA_START_POS(0.0f, 4.0f, 12.0f);
const float     CAMERA_START_YAW = -90.0f;
const float     CAMERA_START_PITCH = -15.0f;
const float     START_FOV = 45.0f;

const float     START_ORTHO_HALF_HEIGHT = 6.0f;

glm::vec3 g_cameraPos = CAMERA_START_POS;
glm::vec3 g_cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 g_cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 g_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);

float g_yaw = CAMERA_START_YAW;
float g_pitch = CAMERA_START_PITCH;
float g_fov = START_FOV;
float g_orthoHalfHeight = START_ORTHO_HALF_HEIGHT;


const float NEAR_PLANE = 0.1f;
const float FAR_PLANE = 100.0f;

const float MOVE_SPEED = 5.0f;
const float SPRINT_MULTIPLIER = 3.0f;
const float MOUSE_SENSITIVITY = 0.1f;
const float ARROW_TURN_SPEED = 90.0f;

bool   g_mouseLookActive = false;
bool   g_firstMouse = true;
double g_lastMouseX = 0.0;
double g_lastMouseY = 0.0;

bool g_wireframe = false;
bool g_printMatricesRequested = false;

const char* vertexShaderSource = R"(
#version 430 core

layout (location = 0) in vec3 aPos;     
layout (location = 1) in vec3 aColor;   

out vec3 vColor;                        

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    gl_Position = u_Projection * u_View * u_Model * vec4(aPos, 1.0);

    vColor = aColor;
}
)";


const char* fragmentShaderSource = R"(
#version 430 core

in vec3 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
)";


const float CUBE_VERTICES[] = {
    // positions             // colours
    // Front face (+Z) - red
    -0.5f, -0.5f,  0.5f,     0.9f, 0.2f, 0.2f,
     0.5f, -0.5f,  0.5f,     0.9f, 0.2f, 0.2f,
     0.5f,  0.5f,  0.5f,     0.9f, 0.2f, 0.2f,
     0.5f,  0.5f,  0.5f,     0.9f, 0.2f, 0.2f,
    -0.5f,  0.5f,  0.5f,     0.9f, 0.2f, 0.2f,
    -0.5f, -0.5f,  0.5f,     0.9f, 0.2f, 0.2f,

    // Back face (-Z) - green
     0.5f, -0.5f, -0.5f,     0.2f, 0.8f, 0.3f,
    -0.5f, -0.5f, -0.5f,     0.2f, 0.8f, 0.3f,
    -0.5f,  0.5f, -0.5f,     0.2f, 0.8f, 0.3f,
    -0.5f,  0.5f, -0.5f,     0.2f, 0.8f, 0.3f,
     0.5f,  0.5f, -0.5f,     0.2f, 0.8f, 0.3f,
     0.5f, -0.5f, -0.5f,     0.2f, 0.8f, 0.3f,

     // Left face (-X) - blue
     -0.5f, -0.5f, -0.5f,     0.2f, 0.4f, 0.9f,
     -0.5f, -0.5f,  0.5f,     0.2f, 0.4f, 0.9f,
     -0.5f,  0.5f,  0.5f,     0.2f, 0.4f, 0.9f,
     -0.5f,  0.5f,  0.5f,     0.2f, 0.4f, 0.9f,
     -0.5f,  0.5f, -0.5f,     0.2f, 0.4f, 0.9f,
     -0.5f, -0.5f, -0.5f,     0.2f, 0.4f, 0.9f,

     // Right face (+X) - yellow
      0.5f, -0.5f,  0.5f,     0.95f, 0.85f, 0.2f,
      0.5f, -0.5f, -0.5f,     0.95f, 0.85f, 0.2f,
      0.5f,  0.5f, -0.5f,     0.95f, 0.85f, 0.2f,
      0.5f,  0.5f, -0.5f,     0.95f, 0.85f, 0.2f,
      0.5f,  0.5f,  0.5f,     0.95f, 0.85f, 0.2f,
      0.5f, -0.5f,  0.5f,     0.95f, 0.85f, 0.2f,

      // Top face (+Y) - cyan
      -0.5f,  0.5f,  0.5f,     0.2f, 0.85f, 0.9f,
       0.5f,  0.5f,  0.5f,     0.2f, 0.85f, 0.9f,
       0.5f,  0.5f, -0.5f,     0.2f, 0.85f, 0.9f,
       0.5f,  0.5f, -0.5f,     0.2f, 0.85f, 0.9f,
      -0.5f,  0.5f, -0.5f,     0.2f, 0.85f, 0.9f,
      -0.5f,  0.5f,  0.5f,     0.2f, 0.85f, 0.9f,

      // Bottom face (-Y) - magenta
      -0.5f, -0.5f, -0.5f,     0.85f, 0.3f, 0.85f,
       0.5f, -0.5f, -0.5f,     0.85f, 0.3f, 0.85f,
       0.5f, -0.5f,  0.5f,     0.85f, 0.3f, 0.85f,
       0.5f, -0.5f,  0.5f,     0.85f, 0.3f, 0.85f,
      -0.5f, -0.5f,  0.5f,     0.85f, 0.3f, 0.85f,
      -0.5f, -0.5f, -0.5f,     0.85f, 0.3f, 0.85f,
};


const float PYRAMID_VERTICES[] = {
    // Base (y = -0.5), dark grey
    -0.5f, -0.5f, -0.5f,     0.3f, 0.3f, 0.3f,
     0.5f, -0.5f, -0.5f,     0.3f, 0.3f, 0.3f,
     0.5f, -0.5f,  0.5f,     0.3f, 0.3f, 0.3f,
     0.5f, -0.5f,  0.5f,     0.3f, 0.3f, 0.3f,
    -0.5f, -0.5f,  0.5f,     0.3f, 0.3f, 0.3f,
    -0.5f, -0.5f, -0.5f,     0.3f, 0.3f, 0.3f,

    // Front side (+Z)
    -0.5f, -0.5f,  0.5f,     1.0f, 0.5f, 0.1f,   // front-left  corner: orange
     0.5f, -0.5f,  0.5f,     0.9f, 0.1f, 0.4f,   // front-right corner: pink
     0.0f,  0.5f,  0.0f,     1.0f, 1.0f, 1.0f,   // tip: white

     // Right side (+X)
      0.5f, -0.5f,  0.5f,     0.9f, 0.1f, 0.4f,
      0.5f, -0.5f, -0.5f,     0.5f, 0.2f, 0.9f,   // back-right corner: purple
      0.0f,  0.5f,  0.0f,     1.0f, 1.0f, 1.0f,

      // Back side (-Z)
       0.5f, -0.5f, -0.5f,     0.5f, 0.2f, 0.9f,
      -0.5f, -0.5f, -0.5f,     0.1f, 0.7f, 0.9f,   // back-left corner: light blue
       0.0f,  0.5f,  0.0f,     1.0f, 1.0f, 1.0f,

       // Left side (-X)
       -0.5f, -0.5f, -0.5f,     0.1f, 0.7f, 0.9f,
       -0.5f, -0.5f,  0.5f,     1.0f, 0.5f, 0.1f,
        0.0f,  0.5f,  0.0f,     1.0f, 1.0f, 1.0f,
};


const float AXES_VERTICES[] = {
    // X axis - red
    0.0f, 0.01f, 0.0f,     1.0f, 0.0f, 0.0f,
    5.0f, 0.01f, 0.0f,     1.0f, 0.0f, 0.0f,
    // Y axis - green
    0.0f, 0.01f, 0.0f,     0.0f, 1.0f, 0.0f,
    0.0f, 5.00f, 0.0f,     0.0f, 1.0f, 0.0f,
    // Z axis - blue
    0.0f, 0.01f, 0.0f,     0.0f, 0.4f, 1.0f,
    0.0f, 0.01f, 5.0f,     0.0f, 0.4f, 1.0f,
};

std::vector<float> createGridVertices(int halfSize)
{
    std::vector<float> v;

    auto pushVertex = [&v](float x, float y, float z, float c) {
        v.push_back(x); v.push_back(y); v.push_back(z);   // position
        v.push_back(c); v.push_back(c); v.push_back(c);   // grey colour (r = g = b)
        };

    for (int i = -halfSize; i <= halfSize; ++i) {
        float colour = (i % 10 == 0) ? 0.55f : 0.30f;   // brighter every 10 units
        float p = static_cast<float>(i);
        float h = static_cast<float>(halfSize);

        // Line parallel to the Z axis (runs "into the screen"), at x = p
        pushVertex(p, 0.0f, -h, colour);
        pushVertex(p, 0.0f, h, colour);

        // Line parallel to the X axis (runs left-right), at z = p
        pushVertex(-h, 0.0f, p, colour);
        pushVertex(h, 0.0f, p, colour);
    }
    return v;
}



struct Mesh
{
    GLuint  vao = 0;              // remembers HOW to read the vertex buffer
    GLuint  vbo = 0;              // the raw vertex bytes stored on the GPU
    GLsizei vertexCount = 0;              // how many vertices glDrawArrays should draw
    GLenum  drawMode = GL_TRIANGLES;   // GL_TRIANGLES for solids, GL_LINES for grid/axes
};

Mesh createMesh(const float* data, size_t sizeInBytes, GLenum drawMode)
{
    Mesh mesh;
    mesh.drawMode = drawMode;
    mesh.vertexCount = static_cast<GLsizei>(sizeInBytes / (6 * sizeof(float)));  // 6 floats per vertex

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeInBytes, data, GL_STATIC_DRAW);

    // Attribute 0 = position: 3 floats, starting at byte 0, stride of 6 floats.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Attribute 1 = colour: 3 floats, starting 3 floats into each vertex, same stride.
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return mesh;
}

void drawMesh(const Mesh& mesh, GLint locModel, const glm::mat4& model)
{
    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(mesh.vao);
    glDrawArrays(mesh.drawMode, 0, mesh.vertexCount);
}

void destroyMesh(Mesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}


void updateCameraVectors()
{
    float yawRad = glm::radians(g_yaw);     // std::sin / std::cos work in RADIANS
    float pitchRad = glm::radians(g_pitch);

    glm::vec3 front;
    front.x = std::cos(yawRad) * std::cos(pitchRad);
    front.y = std::sin(pitchRad);
    front.z = std::sin(yawRad) * std::cos(pitchRad);
    g_cameraFront = glm::normalize(front);

    // cross(a, b) gives a vector perpendicular to both a and b.
    // right = perpendicular to "where I look" and "world up"  -> points to my right.
    // up    = perpendicular to "right" and "where I look"      -> the camera's own up.
    g_cameraRight = glm::normalize(glm::cross(g_cameraFront, WORLD_UP));
    g_cameraUp = glm::normalize(glm::cross(g_cameraRight, g_cameraFront));
}

// Pitch must stay below 90 degrees. At exactly +/-90 the camera would look straight up
// or down, "front" would be parallel to WORLD_UP, their cross product would be zero, and
// lookAt() would break (the image flips or disappears).
void clampPitch()
{
    if (g_pitch > 89.0f) g_pitch = 89.0f;
    if (g_pitch < -89.0f) g_pitch = -89.0f;
}

void resetCamera()
{
    g_cameraPos = CAMERA_START_POS;
    g_yaw = CAMERA_START_YAW;
    g_pitch = CAMERA_START_PITCH;
    g_fov = START_FOV;
    g_orthoHalfHeight = START_ORTHO_HALF_HEIGHT;
    updateCameraVectors();
}

void printMatrix(const char* name, const glm::mat4& m)
{
    std::cout << name << ":\n" << std::fixed << std::setprecision(3);
    for (int row = 0; row < 4; ++row) {
        std::cout << "  [";
        for (int col = 0; col < 4; ++col)
            std::cout << std::setw(9) << m[col][row];
        std::cout << " ]\n";
    }
}


void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
{
    g_screenWidth = width;
    g_screenHeight = height;
    glViewport(0, 0, width, height);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    if (button != GLFW_MOUSE_BUTTON_RIGHT) return;

    if (action == GLFW_PRESS) {
        g_mouseLookActive = true;
        g_firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else if (action == GLFW_RELEASE) {
        g_mouseLookActive = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void cursorPosCallback(GLFWwindow* /*window*/, double xpos, double ypos)
{
    if (!g_mouseLookActive) return;

    if (g_firstMouse) {
        g_lastMouseX = xpos;
        g_lastMouseY = ypos;
        g_firstMouse = false;
        return;
    }

    float xOffset = static_cast<float>(xpos - g_lastMouseX);
    float yOffset = static_cast<float>(g_lastMouseY - ypos);
    g_lastMouseX = xpos;
    g_lastMouseY = ypos;

    g_yaw += xOffset * MOUSE_SENSITIVITY;
    g_pitch += yOffset * MOUSE_SENSITIVITY;
    clampPitch();
    updateCameraVectors();
}


void scrollCallback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
    g_fov -= static_cast<float>(yoffset) * 2.0f;
    g_orthoHalfHeight -= static_cast<float>(yoffset) * 0.5f;

    g_fov = glm::clamp(g_fov, 10.0f, 100.0f);
    g_orthoHalfHeight = glm::clamp(g_orthoHalfHeight, 1.0f, 30.0f);
}

void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, true);
        break;

    case GLFW_KEY_F:
        g_wireframe = !g_wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);
        break;

    case GLFW_KEY_R:
        resetCamera();
        break;

    case GLFW_KEY_V:
        g_printMatricesRequested = true;
        break;
    }
}


void processInput(GLFWwindow* window, float deltaTime)
{
    float speed = MOVE_SPEED * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= SPRINT_MULTIPLIER;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) g_cameraPos += g_cameraFront * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) g_cameraPos -= g_cameraFront * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) g_cameraPos += g_cameraRight * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) g_cameraPos -= g_cameraRight * speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) g_cameraPos += WORLD_UP * speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) g_cameraPos -= WORLD_UP * speed;

    float turn = ARROW_TURN_SPEED * deltaTime;
    bool turned = false;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { g_yaw -= turn; turned = true; }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { g_yaw += turn; turned = true; }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { g_pitch += turn; turned = true; }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) { g_pitch -= turn; turned = true; }
    if (turned) {
        clampPitch();
        updateCameraVectors();
    }
}


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

    GLFWwindow* window = glfwCreateWindow(g_screenWidth, g_screenHeight, "MVP Camera Demo", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);   // v-sync

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glViewport(0, 0, g_screenWidth, g_screenHeight);


    glEnable(GL_DEPTH_TEST);


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

    GLint locModel = glGetUniformLocation(shaderProgram, "u_Model");
    GLint locView = glGetUniformLocation(shaderProgram, "u_View");
    GLint locProjection = glGetUniformLocation(shaderProgram, "u_Projection");


    Mesh cubeMesh = createMesh(CUBE_VERTICES, sizeof(CUBE_VERTICES), GL_TRIANGLES);
    Mesh pyramidMesh = createMesh(PYRAMID_VERTICES, sizeof(PYRAMID_VERTICES), GL_TRIANGLES);
    Mesh axesMesh = createMesh(AXES_VERTICES, sizeof(AXES_VERTICES), GL_LINES);

    std::vector<float> gridVertices = createGridVertices(40);
    Mesh gridMesh = createMesh(gridVertices.data(), gridVertices.size() * sizeof(float), GL_LINES);

    updateCameraVectors();

    float lastFrame = static_cast<float>(glfwGetTime());
    float lastTitleUpdate = 0.0f;

    std::cout <<
        "Controls:\n"
        "  W/S forward/back   A/D left/right   E/Q up/down   Left Shift = faster\n"
        "  Hold RIGHT mouse button + move mouse to look around (or use the arrow keys)\n"
        "  Scroll = zoom   F = wireframe   V = print matrices   R = reset camera   Esc = quit\n\n";

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float t = currentFrame;

        processInput(window, deltaTime);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // glm::lookAt(eye, target, up) builds the view matrix from:
        //   eye    = where the camera is
        //   target = a point the camera looks AT
        //   up     = which way is "up" for the camera

        glm::mat4 view = glm::lookAt(g_cameraPos, g_cameraPos + g_cameraFront, g_cameraUp);

        float aspect = (g_screenHeight > 0)
            ? static_cast<float>(g_screenWidth) / static_cast<float>(g_screenHeight)
            : 1.0f;

        float orthoHalfH = g_orthoHalfHeight;
        float orthoHalfW = g_orthoHalfHeight * aspect;

        glm::mat4 projection = glm::ortho(-orthoHalfW, orthoHalfW, -orthoHalfH, orthoHalfH, NEAR_PLANE, FAR_PLANE);
        const char* projectionName = "ORTHOGRAPHIC";

        //glm::mat4 projection = glm::perspective(glm::radians(g_fov), aspect, NEAR_PLANE, FAR_PLANE);
        // const char* projectionName = "PERSPECTIVE";

        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(projection));

        if (g_printMatricesRequested) {
            std::cout << "\n===== " << projectionName << " =====\n";
            printMatrix("VIEW matrix", view);
            printMatrix("PROJECTION matrix", projection);
            g_printMatricesRequested = false;
        }


        glm::mat4 identity = glm::mat4(1.0f);
        drawMesh(gridMesh, locModel, identity);
        drawMesh(axesMesh, locModel, identity);

        //two rows of identical pillars running away from the camera.
        for (int i = 0; i < 10; ++i) {
            float z = -4.0f * static_cast<float>(i);

            for (float x : { -4.0f, 4.0f }) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(x, 1.5f, z));
                model = glm::scale(model, glm::vec3(1.0f, 3.0f, 1.0f));
                drawMesh(cubeMesh, locModel, model);
            }
        }

        //spinning cube
        glm::vec3 spinnerCentre(0.0f, 1.5f, -3.0f);
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, spinnerCentre);
            model = glm::rotate(model, t * glm::radians(45.0f),
                glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
            drawMesh(cubeMesh, locModel, model);
        }

        //small orbiting cube
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, spinnerCentre);
            model = glm::rotate(model, t * glm::radians(90.0f), WORLD_UP);
            model = glm::translate(model, glm::vec3(2.5f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.4f));
            drawMesh(cubeMesh, locModel, model);
        }

        //pyramid in the middle
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 1.0f, -12.0f));
            model = glm::scale(model, glm::vec3(2.0f));
            drawMesh(pyramidMesh, locModel, model);
        }

        //far away cube
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.5f, -26.0f));
            drawMesh(cubeMesh, locModel, model);
        }

        if (currentFrame - lastTitleUpdate > 0.25f) {
            char title[256];
            std::snprintf(title, sizeof(title),
                "%s | pos (%.1f, %.1f, %.1f) | yaw %.0f  pitch %.0f | FOV %.0f | ortho half-height %.1f",
                projectionName,
                g_cameraPos.x, g_cameraPos.y, g_cameraPos.z,
                g_yaw, g_pitch, g_fov, g_orthoHalfHeight);
            glfwSetWindowTitle(window, title);
            lastTitleUpdate = currentFrame;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroyMesh(cubeMesh);
    destroyMesh(pyramidMesh);
    destroyMesh(axesMesh);
    destroyMesh(gridMesh);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}