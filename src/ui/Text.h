#pragma once
#include <string>
#include <glm/glm.hpp>

// On-screen text (owner C, req 8). FreeType only (CLAUDE.md), following
// learnopengl In-Practice/Text-Rendering (or 2D-Game/Render-text). FreeType is already built and linked.
// Positions are world units; (x, y) is the left end of the baseline.
namespace Text
{
    bool init();       // load the font from assets/fonts/ (relative path)
    void shutdown();

    void draw(const std::string& text, float x, float y, float scale, const glm::vec4& color);
    void drawCentered(const std::string& text, float centerX, float y, float scale, const glm::vec4& color);
    float width(const std::string& text, float scale);
}
