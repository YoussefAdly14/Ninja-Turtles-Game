#pragma once

// Sewer background + environment elements (req 111-118). Shared by the menus, the game and the cutscene.
// Spec needs >= 5 distinct elements integrated naturally, never covering lanes, characters or health bars.
namespace Environment
{
    void init();
    void shutdown();

    void drawBackground(float time);   // owner D: tunnel wall, arched brick ceiling, water channel
    void drawProps(float time);        // owners A + C: one function per element (see Environment.cpp)
}
