#pragma once

struct World;

// Everything that happens when two things in the same lane touch (owner B).
// Called once per frame by World::update, after everything has moved.
namespace Combat
{
    void resolve(World& world);
}
