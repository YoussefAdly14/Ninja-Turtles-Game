#pragma once

// The turtles' Lair, home base at the left edge across every lane (owner D, req 40).
// Must be distinct from the environment by SHAPE: round vault door, shell emblem, pipes, lights.
namespace LairModel
{
    void init();
    void shutdown();
    void draw(float time);   // world coordinates: x 0..LAIR_WIDTH, y PLAYFIELD_BOTTOM..PLAYFIELD_TOP
                             // (+ a footing down into the water channel and the top up under the HUD, for the cutscene)
}
