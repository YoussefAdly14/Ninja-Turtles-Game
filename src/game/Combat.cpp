#include "game/Combat.h"
#include "game/World.h"
#include "audio/Audio.h"
#include "core/Colors.h"
#include "Config.h"
#include <cmath>

// Same lane + boxes overlap on both axes: the AABB test from learnopengl In-Practice/2D-Game/Collisions
// (CheckCollision), with the lane check from overlapsX.
static bool overlapsBox(const Entity& a, const Entity& b)
{
    return overlapsX(a, b) && a.bottom() < b.top() && b.bottom() < a.top();
}

// Band check (req 44, 58): a HIGH shot (shuriken) flies over a ducking turtle, a LOW shot (blaster)
// under a jumping one; standing turtles are hit by both. Decided by the turtle's state for the whole
// jump/duck, not just the height of its box, so a jump started a moment early or late still clears
// the bolt. Donatello never jumps or ducks, so both bands always hit him.
static bool dodges(const Turtle& t, AttackBand band)
{
    if (band == AttackBand::High) return t.ducking;
    if (band == AttackBand::Low) return t.jumpTimer > 0.0f;
    return false;   // Mid: nobody dodges
}

// Damage + hit flash + sound, for turtles and villains alike (req 82, 83, 94).
template <typename T>
static void hurt(T& target, int amount)
{
    target.takeDamage(amount);   // alive = false at 0 HP, removed by World::removeDead at frame end
    target.hitFlash = 1.0f;
    Audio::play(target.alive ? Sound::Hit : Sound::Death);
}

// Hit sparks (req 120), spawned next to every hurt(): purely visual. Turtle hits glow mutagen green;
// villain hits take the colour of what hit (steel shuriken, orange blaster, yellow punch).
const glm::vec4 SPARK_TURTLE_HIT = MUTAGEN_GLOW;
const glm::vec4 SPARK_SHURIKEN   = hexColor(0xDDE8FF);
const glm::vec4 SPARK_BLASTER    = hexColor(0xFF8A3D);
const glm::vec4 SPARK_PUNCH      = hexColor(0xFFE27A);

// Req 79: Donatello shields the turtles behind him (same lane, between him and the Lair) from
// villain attacks. "Walked past him" = the turtle's centre is beyond his front (right) edge, so a
// turtle that was just placed next to him and got stopped by a villain he is blocking stays covered.
// Once it walks past him, or he dies, it can be hit again.
// Donatello can stand anywhere in the lane, so he only shields against an attacker on his right
// (attackerX = the villain's or the shot's x): an attacker already to his left is on the Lair side of him.
static bool isShielded(const World& world, const Turtle& t, float attackerX)
{
    if (t.kind == TurtleKind::Donatello) return false;   // the blocker always takes the hit himself
    for (const Turtle& d : world.turtles)
        if (d.alive && d.kind == TurtleKind::Donatello && d.lane == t.lane && t.x <= d.right() && attackerX > d.x) return true;
    return false;
}

// Contact rule (req 48, OQ7): a turtle and a villain whose hitboxes overlap in the same lane
// both stop and keep attacking until one dies; recomputed every frame, so the survivor walks on
// as soon as the other is gone. Turtles pass through turtles, villains through villains (req 80, 51).
// Donatello blocks too.
static void updateFighting(World& world)
{
    for (Turtle& t : world.turtles) t.fighting = false;
    for (Villain& v : world.villains) v.fighting = false;

    for (Turtle& t : world.turtles) {
        if (!t.alive) continue;
        for (Villain& v : world.villains) {
            if (v.alive && overlapsX(t, v)) {
                t.fighting = true;
                v.fighting = true;
            }
        }
    }
}

// Melee lands on the swing's strike frame, on the NEAREST opponent in reach (req 48, 71, 90).
// The attacker itself may have died earlier this frame: both hits of a trade still land.
// Rocksteady's punch ignores jump/duck (OQ4) but not Donatello's shield.
static void meleeStrikes(World& world)
{
    for (Turtle& t : world.turtles) {
        if (!t.meleeStrike) continue;
        t.meleeStrike = false;
        Villain* target = nullptr;
        for (Villain& v : world.villains) {
            if (!v.alive || !overlapsX(t, v, LEONARDO_REACH)) continue;
            if (!target || std::fabs(v.x - t.x) < std::fabs(target->x - t.x)) target = &v;
        }
        if (target) {
            hurt(*target, TURTLE_DAMAGE);
            // katana sparks on the villain's front (left) edge, flying on through him
            world.effects.hitSparks(glm::vec2(target->left(), target->bottom() + target->height * 0.6f), 1.0f, SPARK_TURTLE_HIT);
        }
    }

    for (Villain& v : world.villains) {
        if (!v.meleeStrike) continue;
        v.meleeStrike = false;
        Turtle* target = nullptr;
        for (Turtle& t : world.turtles) {
            if (!t.alive || isShielded(world, t, v.x) || !overlapsX(v, t, ROCKSTEADY_REACH)) continue;
            if (!target || std::fabs(t.x - v.x) < std::fabs(target->x - v.x)) target = &t;
        }
        if (target) {
            hurt(*target, ROCKSTEADY_DAMAGE);
            // heavy punch: bigger burst on the turtle's front (right) edge
            world.effects.hitSparks(glm::vec2(target->right(), target->bottom() + target->height * 0.55f), -1.0f, SPARK_PUNCH, 1.3f);
        }
    }
}

// Projectile hits (req 44, 49, 50, 58, 75, 79). Shots never touch each other (req 49), turtle shots
// never hurt turtles and villain shots never hurt villains. A shot hits only the FIRST target it
// touches (no piercing), then plays its impact flash and is removed.
static void projectileHits(World& world)
{
    for (Projectile& p : world.projectiles) {
        if (!p.alive || p.hasHit) continue;

        if (p.fromTurtle) {
            // sai flies right: the first villain it reaches is the one with the leftmost edge
            Villain* target = nullptr;
            for (Villain& v : world.villains) {
                if (!v.alive || !overlapsBox(p, v)) continue;
                if (!target || v.left() < target->left()) target = &v;
            }
            if (target) {
                hurt(*target, p.damage);
                p.impact();
                world.effects.hitSparks(glm::vec2(p.x, p.bottom() + p.height * 0.5f), 1.0f, SPARK_TURTLE_HIT);
            }
        }
        else {
            // villain shots fly left: the first turtle reached is the one with the rightmost edge
            // (Donatello wins a tie). Turtles behind Donatello are skipped: he takes it (req 79).
            Turtle* target = nullptr;
            for (Turtle& t : world.turtles) {
                if (!t.alive || isShielded(world, t, p.x) || !overlapsBox(p, t) || dodges(t, p.band)) continue;
                if (!target || t.right() > target->right()
                    || (t.right() == target->right() && t.kind == TurtleKind::Donatello))
                    target = &t;
            }
            if (target) {
                hurt(*target, p.damage);
                p.impact();
                glm::vec4 spark = p.kind == ProjectileKind::BlasterBolt ? SPARK_BLASTER : SPARK_SHURIKEN;
                world.effects.hitSparks(glm::vec2(p.x, p.bottom() + p.height * 0.5f), -1.0f, spark);
            }
        }
    }
}

// A turtle touching a pizza collects it (req 106, OQ13: only turtles collect, no timeout).
static void collectPizza(World& world)
{
    for (Collectible& c : world.collectibles) {
        if (!c.alive) continue;
        for (const Turtle& t : world.turtles) {
            if (t.alive && overlapsBox(t, c)) {
                world.economy.add(c.value);
                c.alive = false;
                Audio::play(Sound::Pickup);
                break;
            }
        }
    }
}

// A villain that died this frame drops a pizza where it fell (req 95, 105). removeDead() erases it
// at the end of this same frame, so each villain drops exactly once. Kept inside the screen so a
// villain killed while still walking in doesn't leave an unreachable slice off the right edge.
static void dropLoot(World& world)
{
    for (const Villain& v : world.villains) {
        if (v.alive) continue;
        float dropX = v.x < WORLD_WIDTH - PIZZA_SIZE ? v.x : WORLD_WIDTH - PIZZA_SIZE;
        // harder villains + later waves drop more (Config: *_PIZZA_VALUE, PIZZA_WAVE_BONUS)
        world.dropPizza(v.lane, dropX, pizzaValue(v.kind, world.waves.waveNumber()));   // pushes into collectibles: safe in this loop
    }
}

// Defeat effects (req 83, 120) for everything that died this frame: shell spin for turtles, smoke +
// dropped weapon for villains. Visual only: removeDead() still erases them at the end of this frame,
// so the lane frees up at once. hp > 0 = a turtle that just walked off-screen, no effect for that.
static void defeatEffects(World& world)
{
    for (const Turtle& t : world.turtles)
        if (!t.alive && t.hp <= 0)
            world.effects.turtleDefeat(t.kind, glm::vec2(t.x, t.bottom() + t.height * 0.5f), laneFloorY(t.lane));
    for (const Villain& v : world.villains)
        if (!v.alive && v.hp <= 0)
            world.effects.villainDefeat(v.kind, glm::vec2(v.x, v.bottom() + v.height * 0.5f), v.height, laneFloorY(v.lane));
}

void Combat::resolve(World& world)
{
    updateFighting(world);
    meleeStrikes(world);
    projectileHits(world);
    collectPizza(world);
    dropLoot(world);
    defeatEffects(world);
}
