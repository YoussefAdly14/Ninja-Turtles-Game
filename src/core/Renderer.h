#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include "core/Mesh.h"

// The building-block shapes every model is made from (see Mesh.h for their sizes).
enum class Shape { Quad, Triangle, RightTriangle, Circle, Semicircle, Ring, Count };

// One flat-colour shader (assets/shaders/basic.*) + the building-block meshes.
// All positions are world units: (0,0) bottom-left, (WORLD_WIDTH, WORLD_HEIGHT) top-right.
namespace Renderer
{
    bool init();       // after the GL context exists
    void shutdown();

    void beginFrame(const glm::mat4& projection);   // also resets view, tint and grayscale
    void setView(const glm::mat4& view);            // camera pan/zoom for the cutscene (identity in game)

    // Fragment-shader effects, applied to everything drawn until changed back.
    void setDamageTint(float amount);   // 0..1: red damage feedback (req 82, 94)
    void setGrayscale(float amount);    // 0..1: locked cards (req 62)

    void draw(Shape shape, const glm::mat4& model, const glm::vec4& color);
    void draw(const Mesh& mesh, const glm::mat4& model, const glm::vec4& color);   // your own meshes

    // shortcuts in world units
    void drawRect(glm::vec2 center, glm::vec2 size, const glm::vec4& color, float angleDegrees = 0.0f);
    void drawCircle(glm::vec2 center, float radius, const glm::vec4& color);
    void drawLine(glm::vec2 a, glm::vec2 b, float thickness, const glm::vec4& color);
    void drawRoundedRect(glm::vec2 center, glm::vec2 size, float radius, const glm::vec4& color);   // opaque colours only (parts overlap)
}
