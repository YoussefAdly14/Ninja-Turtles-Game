#include "core/Renderer.h"
#include "core/Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>

const int CIRCLE_SEGMENTS = 48;
const float RING_INNER_RADIUS = 0.4f;

static Shader* g_shader = nullptr;
static Mesh g_shapes[(int)Shape::Count];

// uniform locations, looked up once (Lab 4)
static GLint locModel = -1;
static GLint locView = -1;
static GLint locProjection = -1;
static GLint locColor = -1;
static GLint locDamageTint = -1;
static GLint locGrayscale = -1;

bool Renderer::init()
{
    g_shader = new Shader("assets/shaders/basic.vert", "assets/shaders/basic.frag");

    locModel      = glGetUniformLocation(g_shader->ID, "u_Model");
    locView       = glGetUniformLocation(g_shader->ID, "u_View");
    locProjection = glGetUniformLocation(g_shader->ID, "u_Projection");
    locColor      = glGetUniformLocation(g_shader->ID, "u_Color");
    locDamageTint = glGetUniformLocation(g_shader->ID, "u_DamageTint");
    locGrayscale  = glGetUniformLocation(g_shader->ID, "u_Grayscale");
    if (locModel == -1 || locColor == -1) {
        std::cerr << "Renderer: basic shader failed to load (is the game running next to assets/?)\n";
        return false;
    }

    g_shapes[(int)Shape::Quad]          = createQuadMesh();
    g_shapes[(int)Shape::Triangle]      = createTriangleMesh();
    g_shapes[(int)Shape::RightTriangle] = createRightTriangleMesh();
    g_shapes[(int)Shape::Circle]        = createCircleMesh(CIRCLE_SEGMENTS);
    g_shapes[(int)Shape::Semicircle]    = createSemicircleMesh(CIRCLE_SEGMENTS / 2);
    g_shapes[(int)Shape::Ring]          = createRingMesh(CIRCLE_SEGMENTS, RING_INNER_RADIUS);

    // transparency for lane highlights, fades, dim overlays (learnopengl Advanced-OpenGL/Blending)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return true;
}

void Renderer::shutdown()
{
    for (Mesh& m : g_shapes) destroyMesh(m);
    if (g_shader) {
        glDeleteProgram(g_shader->ID);
        delete g_shader;
        g_shader = nullptr;
    }
}

void Renderer::beginFrame(const glm::mat4& projection)
{
    g_shader->use();
    glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(projection));
    setView(glm::mat4(1.0f));
    setDamageTint(0.0f);
    setGrayscale(0.0f);
}

void Renderer::setView(const glm::mat4& view)
{
    glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
}

void Renderer::setDamageTint(float amount)
{
    glUniform1f(locDamageTint, amount);
}

void Renderer::setGrayscale(float amount)
{
    glUniform1f(locGrayscale, amount);
}

void Renderer::draw(Shape shape, const glm::mat4& model, const glm::vec4& color)
{
    draw(g_shapes[(int)shape], model, color);
}

void Renderer::draw(const Mesh& mesh, const glm::mat4& model, const glm::vec4& color)
{
    glUniform4f(locColor, color.r, color.g, color.b, color.a);
    drawMesh(mesh, locModel, model);
}

void Renderer::drawRect(glm::vec2 center, glm::vec2 size, const glm::vec4& color, float angleDegrees)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(center, 0.0f));
    model = glm::rotate(model, glm::radians(angleDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(size, 1.0f));
    draw(Shape::Quad, model, color);
}

void Renderer::drawCircle(glm::vec2 center, float radius, const glm::vec4& color)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(center, 0.0f));
    model = glm::scale(model, glm::vec3(radius * 2.0f, radius * 2.0f, 1.0f));
    draw(Shape::Circle, model, color);
}

// a thick line is a quad stretched from a to b
void Renderer::drawLine(glm::vec2 a, glm::vec2 b, float thickness, const glm::vec4& color)
{
    glm::vec2 d = b - a;
    float angle = atan2f(d.y, d.x);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3((a + b) * 0.5f, 0.0f));
    model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(glm::length(d), thickness, 1.0f));
    draw(Shape::Quad, model, color);
}
