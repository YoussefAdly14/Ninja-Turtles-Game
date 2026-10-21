#include "game/Turtle.h"
#include "game/World.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "models/TurtleModels.h"
#include "Config.h"

Turtle makeTurtle(TurtleKind kind, int lane, int id)
{
    Turtle t;
    t.id = id;
    t.kind = kind;
    t.lane = lane;
    t.x = TURTLE_SPAWN_X;
    t.width = TURTLE_WIDTH;
    t.height = TURTLE_HEIGHT;
    switch (kind) {
    case TurtleKind::Leonardo:  t.maxHp = LEONARDO_HP;  break;
    case TurtleKind::Raphael:   t.maxHp = RAPHAEL_HP;   break;
    case TurtleKind::Donatello: t.maxHp = DONATELLO_HP; break;
    default: break;
    }
    t.hp = t.maxHp;
    t.attackTimer = 0.0f;   // attack as soon as placed (req 47)
    return t;
}

const char* turtleName(TurtleKind kind)
{
    switch (kind) {
    case TurtleKind::Leonardo:  return "Leonardo";
    case TurtleKind::Raphael:   return "Raphael";
    case TurtleKind::Donatello: return "Donatello";
    default:                    return "";
    }
}

// TODO(B): this is a first pass so the skeleton runs. Owner B finishes reqs 45-48, 71, 74, 78-84.
void Turtle::update(float dt, World& world)
{
    age += dt;
    if (hitFlash > 0.0f) hitFlash -= dt * 4.0f;
    if (attackAnim > 0.0f) attackAnim -= dt * 3.0f;

    // jump arc: parabola over JUMP_DURATION
    if (jumpTimer > 0.0f) {
        jumpTimer -= dt;
        if (jumpTimer < 0.0f) jumpTimer = 0.0f;
        float t = 1.0f - jumpTimer / JUMP_DURATION;
        yOffset = JUMP_HEIGHT * 4.0f * t * (1.0f - t);
    }
    else {
        yOffset = 0.0f;
    }
    if (duckTimer > 0.0f) duckTimer -= dt;
    height = ducking ? TURTLE_HEIGHT * DUCK_HEIGHT_SCALE : TURTLE_HEIGHT;

    // walk right unless fighting (req 45, 48)
    if (canMove() && !fighting) {
        x += TURTLE_WALK_SPEED * dt;
        walkCycle += dt * 8.0f;
    }
    if (left() > WORLD_WIDTH) alive = false;   // OQ3: walks off-screen and is removed

    // attack on an interval from the moment it is placed (req 47)
    attackTimer -= dt;
    if (attackTimer <= 0.0f) {
        switch (kind) {
        case TurtleKind::Leonardo:
            attackTimer = LEONARDO_ATTACK_INTERVAL;
            attackAnim = 1.0f;
            // TODO(B): damage the nearest villain within LEONARDO_REACH on the strike frame (req 71)
            break;
        case TurtleKind::Raphael:
            attackTimer = RAPHAEL_ATTACK_INTERVAL;
            attackAnim = 1.0f;
            world.spawnProjectile(ProjectileKind::Sai, lane, right(), true);
            break;
        case TurtleKind::Donatello:
            attackTimer = 1e9f;   // never attacks (req 78)
            break;
        default: break;
        }
    }
}

Pose Turtle::pose() const
{
    Pose p;
    p.time = age;
    p.walkCycle = walkCycle;
    p.attack = attackAnim > 0.0f ? 1.0f - attackAnim : 0.0f;
    p.ducking = ducking;
    p.inAir = jumpTimer > 0.0f;
    p.hitFlash = hitFlash > 0.0f ? hitFlash : 0.0f;
    return p;
}

void Turtle::draw() const
{
    glm::mat4 base = translate2D(glm::mat4(1.0f), x, bottom());
    Renderer::setDamageTint(damageTint());
    TurtleModels::draw(kind, base, pose());
    Renderer::setDamageTint(0.0f);
}

void applyJumpDuck(std::vector<Turtle>& turtles, const LaneManager& lanes, bool jumpPressed, bool duckHeld)
{
    for (Turtle& t : turtles) {
        if (t.kind == TurtleKind::Donatello) continue;   // defense ignores jump/duck
        bool selected = lanes.isSelected(t.lane);
        bool onGround = t.jumpTimer <= 0.0f;

        if (selected && jumpPressed && onGround && !t.ducking)
            t.jumpTimer = JUMP_DURATION;

        if (selected && duckHeld && onGround) {
            t.ducking = true;
            t.duckTimer = DUCK_MIN_DURATION;
        }
        else if (t.duckTimer <= 0.0f) {
            t.ducking = false;
        }
    }
}
