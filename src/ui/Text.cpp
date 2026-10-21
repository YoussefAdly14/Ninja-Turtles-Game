#include "ui/Text.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>

// TODO(C): STUB. Implement learnopengl Text-Rendering here:
//  1. FT_Init_FreeType + FT_New_Face("assets/fonts/<font>.ttf") + FT_Set_Pixel_Sizes
//  2. one GL_RED texture per glyph (glPixelStorei(GL_UNPACK_ALIGNMENT, 1)), store Character{tex, size, bearing, advance}
//  3. its own text shader (assets/shaders/text.vert/.frag) using Window::projection() so text is in world units
//  4. a dynamic VBO of 6 vertices per glyph (GL_DYNAMIC_DRAW + glBufferSubData), as on the page
// Use an original/free font (OFL) and credit it in the README (NF20). Until then text draws nothing.

bool Text::init()
{
    // proves FreeType is linked; replace with the real setup
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library\n";
        return false;
    }
    FT_Done_FreeType(ft);
    return true;
}

void Text::shutdown() {}

void Text::draw(const std::string& /*text*/, float /*x*/, float /*y*/, float /*scale*/, const glm::vec4& /*color*/) {}

void Text::drawCentered(const std::string& text, float centerX, float y, float scale, const glm::vec4& color)
{
    draw(text, centerX - width(text, scale) * 0.5f, y, scale, color);
}

float Text::width(const std::string& /*text*/, float /*scale*/)
{
    return 0.0f;
}
