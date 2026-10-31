#pragma once
#include <vector>
#include "game/Types.h"
#include "game/LaneManager.h"
#include "game/Economy.h"
#include "game/WaveManager.h"
#include "game/Turtle.h"
#include "game/Villain.h"
#include "game/Projectile.h"
#include "game/Collectible.h"
#include "game/Effects.h"

// Everything that exists during one game. A new game = World::reset() (req 20, 25).
// Game owns one World and runs the state machine around it; gameplay code only needs World.
struct World
{
    LaneManager lanes;
    Economy economy;
    WaveManager waves;

    std::vector<Turtle> turtles;
    std::vector<Villain> villains;
    std::vector<Projectile> projectiles;
    std::vector<Collectible> collectibles;
    Effects effects;       // smoke, sparks, defeat effects: visual only (req 68, 83, 120)

    float time = 0.0f;     // seconds of gameplay (frozen while paused)
    int nextId = 1;

    void reset();
    void update(float dt);

    // Spawning: push_back into a DIFFERENT vector than the one you're looping over is fine.
    bool placeTurtle(TurtleKind kind, int lane);   // buys it; false if no lane or not enough pizza
    void spawnVillain(VillainKind kind, int lane);
    void spawnProjectile(ProjectileKind kind, int lane, float x, bool fromTurtle);
    void dropPizza(int lane, float x, int value);

    bool villainReachedLair() const;   // -> lose (req 97)
    bool allWavesCleared() const;      // -> win (req 107)

private:
    void removeDead();                 // end of frame only (req 10)
};
