#include "ui/Text.h"
#include "core/Shader.h"
#include "core/Window.h"
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>
#include <map>

// Text rendering with FreeType, copied from learnopengl In-Practice/Text-Rendering.
// Changes from the page: the projection is Window::projection() (world units), the colour is a vec4,
// and draw() puts back the shader program the Renderer was using (it only binds its shader once per frame).

const char* const FONT_PATH = "assets/fonts/Bangers-Regular.ttf";   // Bangers, SIL OFL 1.1 (assets/fonts/Bangers-OFL.txt)
const unsigned int FONT_PIXEL_SIZE = 48;                           // scale 1.0 = 48 world units tall

// everything FreeType tells us about one glyph (learnopengl Text-Rendering)
struct Character
{
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // size of glyph
    glm::ivec2   Bearing;    // offset from baseline to left/top of glyph
    unsigned int Advance;    // offset to advance to next glyph (1/64 pixels)
};

static std::map<char, Character> Characters;
static Shader* g_textShader = nullptr;
static unsigned int VAO = 0, VBO = 0;
static GLint locTextColor = -1;

bool Text::init()
{
    g_textShader = new Shader("assets/shaders/text.vert", "assets/shaders/text.frag");
    glm::mat4 projection = Window::projection();
    g_textShader->use();
    glUniformMatrix4fv(glGetUniformLocation(g_textShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(g_textShader->ID, "text"), 0);
    locTextColor = glGetUniformLocation(g_textShader->ID, "textColor");
    glUseProgram(0);

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library\n";
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, FONT_PATH, 0, &face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font " << FONT_PATH << "\n";
        FT_Done_FreeType(ft);
        return false;
    }
    FT_Set_Pixel_Sizes(face, 0, FONT_PIXEL_SIZE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    // first 128 ASCII characters, one GL_RED texture each
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph\n";
            continue;
        }
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                     face->glyph->bitmap.width, face->glyph->bitmap.rows,
                     0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // one dynamic quad (6 vertices x vec4) refilled per glyph
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return true;
}

void Text::shutdown()
{
    for (auto& pair : Characters) glDeleteTextures(1, &pair.second.TextureID);
    Characters.clear();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    VAO = VBO = 0;
    if (g_textShader) {
        glDeleteProgram(g_textShader->ID);
        delete g_textShader;
        g_textShader = nullptr;
    }
}

// learnopengl RenderText, (x, y) = left end of the baseline in world units
void Text::draw(const std::string& text, float x, float y, float scale, const glm::vec4& color)
{
    if (!g_textShader || VAO == 0) return;   // init failed: draw nothing

    // remember the Renderer's program so its next draw call still works (glGetIntegerv: learnopengl Shaders)
    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);

    g_textShader->use();
    glUniform4f(locTextColor, color.r, color.g, color.b, color.a);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(previousProgram);
}

void Text::drawCentered(const std::string& text, float centerX, float y, float scale, const glm::vec4& color)
{
    draw(text, centerX - width(text, scale) * 0.5f, y, scale, color);
}

// same advance sum draw() walks through, without drawing
float Text::width(const std::string& text, float scale)
{
    float w = 0.0f;
    for (char c : text) w += (Characters[c].Advance >> 6) * scale;
    return w;
}
