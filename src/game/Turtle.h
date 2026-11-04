#pragma once
#include <vector>
#include "game/Entity.h"
#include "game/Types.h"
#include "models/Pose.h"

struct World;
class LaneManager;

// Leonardo (melee), Raphael (ranged), Donatello (defense). Spec 3.2.
struct Turtle : Entity
{
    TurtleKind kind = TurtleKind::Leonardo;
    float attackTimer = 0.0f;   // counts down to the next attack
    float attackAnim = 0.0f;    // 1 when an attack starts, falls to 0 (drives Pose::attack)
    bool  strikePending = false; // swing started, katana hasn't landed / sai hasn't left the hand yet
    bool  meleeStrike = false;   // Leonardo's katana reached its strike frame this frame (Combat applies it)
    float jumpTimer = 0.0f;     // > 0 while in the air
    float duckTimer = 0.0f;     // keeps the duck for DUCK_MIN_DURATION after S is released
    bool  ducking = false;
    bool  fighting = false;     // touching a villain: stand still (set by Combat each frame)
    float walkCycle = 0.0f;
    float hitFlash = 0.0f;

    bool canMove() const { return kind != TurtleKind::Donatello; }

    void update(float dt, World& world);
    void draw() const;          // model + damage tint (health bar is drawn separately, on top)
    Pose pose() const;
};

Turtle makeTurtle(TurtleKind kind, int lane, int id, float x = TURTLE_SPAWN_X);
const char* turtleName(TurtleKind kind);

// W/Up = jump, S/Down = duck (held), for every turtle in a selected lane (req 56, 57).
void applyJumpDuck(std::vector<Turtle>& turtles, const LaneManager& lanes, bool jumpPressed, bool duckHeld);
