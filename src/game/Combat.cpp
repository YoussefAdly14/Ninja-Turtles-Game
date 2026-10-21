#include "game/Combat.h"
#include "game/World.h"
#include "Config.h"

// Contact rule (req 48, OQ7): a turtle and a villain whose hitboxes overlap in the same lane
// both stop and keep attacking until one dies. Turtles pass through turtles, villains through
// villains (req 80, 51). Donatello blocks too.
static void updateFighting(World& world)
{
    for (Turtle& t : world.turtles) t.fighting = false;
    for (Villain& v : world.villains) v.fighting = false;

    for (Turtle& t : world.turtles) {
        for (Villain& v : world.villains) {
            if (overlapsX(t, v)) {
                t.fighting = true;
                v.fighting = true;
            }
        }
    }
}

// TODO(B): projectile hits (req 44, 49, 58, 75):
//  - turtle shots hit the FIRST villain they touch, then disappear (no piercing)
//  - villain shots: High is missed by ducking turtles, Low by jumping turtles, Mid hits everyone
//  - Donatello absorbs villain shots aimed at turtles behind him (req 79)
//  - Donatello is unaffected by jump/duck
static void projectileHits(World& /*world*/)
{
}

// TODO(B): melee strike frames for Leonardo / Rocksteady can also be resolved here (req 71, 90).

// TODO(A): a turtle touching a pizza collects it -> world.economy.add(value) (req 106)
static void collectPizza(World& /*world*/)
{
}

// TODO(B/A): when a villain dies, drop a pizza in the same frame (req 95, 105):
//  world.dropPizza(v.lane, v.x);
static void dropLoot(World& /*world*/)
{
}

void Combat::resolve(World& world)
{
    updateFighting(world);
    projectileHits(world);
    collectPizza(world);
    dropLoot(world);
}
