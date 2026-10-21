#include "core/Mesh.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

const float PI = 3.14159265f;

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

    // attribute 0 = position, attribute 1 = colour
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return mesh;
}

Mesh createIndexedMesh(const float* data, size_t sizeInBytes, const unsigned int* indices, size_t indexBytes)
{
    Mesh mesh = createMesh(data, sizeInBytes, GL_TRIANGLES);
    mesh.indexCount = static_cast<GLsizei>(indexBytes / sizeof(unsigned int));

    // the EBO binding is stored in the VAO, so bind the VAO first (Lab 3)
    glBindVertexArray(mesh.vao);
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBytes, indices, GL_STATIC_DRAW);
    glBindVertexArray(0);
    return mesh;
}

void drawMesh(const Mesh& mesh, GLint locModel, const glm::mat4& model)
{
    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(mesh.vao);
    if (mesh.ebo != 0)
        glDrawElements(mesh.drawMode, mesh.indexCount, GL_UNSIGNED_INT, 0);
    else
        glDrawArrays(mesh.drawMode, 0, mesh.vertexCount);
}

void destroyMesh(Mesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    if (mesh.ebo != 0) glDeleteBuffers(1, &mesh.ebo);
    mesh = Mesh();
}

// adds one white vertex
static void pushVertex(std::vector<float>& v, float x, float y)
{
    v.insert(v.end(), { x, y, 0.0f, 1.0f, 1.0f, 1.0f });
}

Mesh createQuadMesh()
{
    const float vertices[] = {
        -0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 1.0f,
         0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 1.0f,
         0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 1.0f,
    };
    const unsigned int indices[] = { 0, 1, 2,   2, 3, 0 };
    return createIndexedMesh(vertices, sizeof(vertices), indices, sizeof(indices));
}

Mesh createTriangleMesh()
{
    const float vertices[] = {
        -0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 1.0f,
         0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 1.0f,
         0.0f,  0.5f, 0.0f,   1.0f, 1.0f, 1.0f,
    };
    return createMesh(vertices, sizeof(vertices), GL_TRIANGLES);
}

Mesh createRightTriangleMesh()
{
    const float vertices[] = {
        -0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 1.0f,
         0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 1.0f,
    };
    return createMesh(vertices, sizeof(vertices), GL_TRIANGLES);
}

// Lab 3 circle: centre vertex + (segments + 1) rim vertices, drawn as a fan
Mesh createCircleMesh(int segments)
{
    std::vector<float> v;
    pushVertex(v, 0.0f, 0.0f);
    for (int i = 0; i <= segments; ++i) {
        float a = 2.0f * PI * i / segments;
        pushVertex(v, 0.5f * cosf(a), 0.5f * sinf(a));
    }
    return createMesh(v.data(), v.size() * sizeof(float), GL_TRIANGLE_FAN);
}

Mesh createSemicircleMesh(int segments)
{
    std::vector<float> v;
    pushVertex(v, 0.0f, 0.0f);
    for (int i = 0; i <= segments; ++i) {
        float a = PI * i / segments;
        pushVertex(v, 0.5f * cosf(a), 0.5f * sinf(a));
    }
    return createMesh(v.data(), v.size() * sizeof(float), GL_TRIANGLE_FAN);
}

// two vertices per step (outer, inner), two triangles per segment
Mesh createRingMesh(int segments, float innerRadius)
{
    std::vector<float> v;
    std::vector<unsigned int> indices;
    for (int i = 0; i <= segments; ++i) {
        float a = 2.0f * PI * i / segments;
        pushVertex(v, 0.5f * cosf(a), 0.5f * sinf(a));
        pushVertex(v, innerRadius * cosf(a), innerRadius * sinf(a));
    }
    for (int i = 0; i < segments; ++i) {
        unsigned int outer = 2 * i, inner = 2 * i + 1;
        unsigned int nextOuter = outer + 2, nextInner = inner + 2;
        indices.insert(indices.end(), { outer, nextOuter, inner,   inner, nextOuter, nextInner });
    }
    return createIndexedMesh(v.data(), v.size() * sizeof(float),
                             indices.data(), indices.size() * sizeof(unsigned int));
}

Mesh createPolygonMesh(const std::vector<glm::vec2>& points)
{
    glm::vec2 centre(0.0f);
    for (const glm::vec2& p : points) centre += p;
    centre /= static_cast<float>(points.size());

    std::vector<float> v;
    pushVertex(v, centre.x, centre.y);
    for (const glm::vec2& p : points) pushVertex(v, p.x, p.y);
    pushVertex(v, points[0].x, points[0].y);   // close the fan
    return createMesh(v.data(), v.size() * sizeof(float), GL_TRIANGLE_FAN);
}
