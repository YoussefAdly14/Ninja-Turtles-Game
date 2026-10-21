#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

// Lab 4 Mesh pattern (Lab4Camera.cpp) + Lab 3 EBO.
// Every vertex is 6 floats: x,y,z, r,g,b. Upload once at startup, never per frame (NF4).
struct Mesh
{
    GLuint  vao = 0;
    GLuint  vbo = 0;
    GLuint  ebo = 0;                  // 0 = no index buffer, drawn with glDrawArrays
    GLsizei vertexCount = 0;
    GLsizei indexCount = 0;
    GLenum  drawMode = GL_TRIANGLES;
};

Mesh createMesh(const float* data, size_t sizeInBytes, GLenum drawMode);
Mesh createIndexedMesh(const float* data, size_t sizeInBytes, const unsigned int* indices, size_t indexBytes);
void drawMesh(const Mesh& mesh, GLint locModel, const glm::mat4& model);
void destroyMesh(Mesh& mesh);

// Building blocks. All are white (u_Color gives the colour) and sized so that
// scale(w, h) in the model matrix gives a w x h shape.
Mesh createQuadMesh();                                   // 1x1 square centred on (0,0), EBO
Mesh createTriangleMesh();                               // 1x1, tip at the top, centred on (0,0)
Mesh createRightTriangleMesh();                          // 1x1, right angle at bottom-left, centred on (0,0)
Mesh createCircleMesh(int segments);                     // diameter 1, centred on (0,0), GL_TRIANGLE_FAN (Lab 3)
Mesh createSemicircleMesh(int segments);                 // diameter 1, flat side on the x axis, dome up
Mesh createRingMesh(int segments, float innerRadius);    // outer radius 0.5, inner radius 0..0.5, EBO

// Custom outline (model units) drawn as a fan around its centre. Works for convex shapes and
// star shapes (every edge visible from the centre): blades, horns, shuriken, pizza slices...
Mesh createPolygonMesh(const std::vector<glm::vec2>& points);
