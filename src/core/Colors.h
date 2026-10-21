#pragma once
#include <glm/glm.hpp>

// 0xRRGGBB -> rgba (0..1), so the theme-bible hex codes can be pasted as-is
inline glm::vec4 hexColor(unsigned int hex, float alpha = 1.0f)
{
    return glm::vec4(((hex >> 16) & 0xFF) / 255.0f, ((hex >> 8) & 0xFF) / 255.0f, (hex & 0xFF) / 255.0f, alpha);
}

inline glm::vec4 withAlpha(glm::vec4 color, float alpha)
{
    color.a = alpha;
    return color;
}

// Theme palette (TMNT.xlsx "TMNT Theme Bible"). Keep turtles and villains in separate families.
const glm::vec4 TURTLE_SKIN    = hexColor(0x4C9A2A);
const glm::vec4 SHELL          = hexColor(0x7A5C2E);
const glm::vec4 PLASTRON       = hexColor(0xD8C27A);
const glm::vec4 LEONARDO_MASK  = hexColor(0x1F5FBF);
const glm::vec4 RAPHAEL_MASK   = hexColor(0xC0262D);
const glm::vec4 DONATELLO_MASK = hexColor(0x7B3FA0);
const glm::vec4 FOOT_CLAN      = hexColor(0x2B1F33);
const glm::vec4 MUTANT_GRAY    = hexColor(0x6E6E6E);   // Bebop / Rocksteady
const glm::vec4 MUTAGEN_GLOW   = hexColor(0x7CFC00);
const glm::vec4 SEWER_WALL     = hexColor(0x3A4A44);
const glm::vec4 PIZZA          = hexColor(0xF2B33D);

const glm::vec4 WHITE = glm::vec4(1.0f);
const glm::vec4 BLACK = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
