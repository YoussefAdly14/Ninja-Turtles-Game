#pragma once

// Dev aid (like req 16/17): every model in every pose on one screen, so modelers can check their
// work without playing. Run: NinjaTurtlesGame.exe --gallery turtles|villains|props
namespace Gallery
{
    int pageFromName(const char* name);   // -1 if unknown
    void draw(int page, float time);
}
