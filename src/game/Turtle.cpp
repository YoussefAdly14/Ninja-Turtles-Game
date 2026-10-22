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

// Movement + attack timing for one turtle (reqs 45-47, 71, 74, 78). Damage between units is applied
// in Combat::resolve after everything has moved.
void Turtle::update(float dt, World& world)
{
    age += dt;
    if (hitFlash > 0.0f) hitFlash -= dt / HIT_FLASH_DURATION;
    if (attackAnim > 0.0f) attackAnim -= dt / ATTACK_ANIM_DURATION;
    meleeStrike = false;

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
    height = ducking ? TURTLE_HEIGHT * DUCK_HEIGHT_SCALE : TURTLE_HEIGHT;   // a lower box ducks under HIGH shots (req 58)

    // walk right unless fighting (req 45, 48)
    if (canMove() && !fighting) {
        x += TURTLE_WALK_SPEED * dt;
        walkCycle += dt * WALK_CYCLE_SPEED;
    }
    if (left() > WORLD_WIDTH) alive = false;   // OQ3: walks off-screen and is removed

    if (kind == TurtleKind::Donatello) return;   // never attacks (req 78)

    // strike frame of the current swing/throw: the katana lands or the sai leaves the hand (req 71, 74)
    if (strikePending && 1.0f - attackAnim >= STRIKE_POINT) {
        strikePending = false;
        if (kind == TurtleKind::Leonardo) meleeStrike = true;   // Combat hits the nearest villain in reach
        else world.spawnProjectile(ProjectileKind::Sai, lane, right(), true);
    }

    // a new attack on an interval from the moment it is placed, even with no villain around (req 47)
    attackTimer -= dt;
    if (attackTimer <= 0.0f) {
        attackTimer = kind == TurtleKind::Leonardo ? LEONARDO_ATTACK_INTERVAL : RAPHAEL_ATTACK_INTERVAL;
        attackAnim = 1.0f;
        strikePending = true;
    }
}

Pose Turtle::pose() const
{
    Pose p;
    p.time = age;
    p.walkCycle = (canMove() && !fighting) ? walkCycle : 0.0f;   // standing still: idle legs, no frozen stride
    p.attack = attackAnim > 0.0f ? 1.0f - attackAnim : 0.0f;
    p.ducking = ducking;
    p.inAir = jumpTimer > 0.0f;
    p.hitFlash = hitFlash > 0.0f ? hitFlash : 0.0f;
    return p;
}

void Turtle::draw() const
{
    glm::mat4 base = translate2D(glm::mat4(1.0f), x, bottom());
    // redder as health drops + a short extra flash right after a hit (req 82)
    float tint = damageTint() + (hitFlash > 0.0f ? hitFlash * HIT_FLASH_TINT : 0.0f);
    Renderer::setDamageTint(tint > 1.0f ? 1.0f : tint);
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
