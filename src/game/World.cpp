#include "game/World.h"
#include "game/Combat.h"
#include "Config.h"
#include <algorithm>

void World::reset()
{
    lanes.reset();
    economy.reset();
    waves.reset();
    turtles.clear();
    villains.clear();
    projectiles.clear();
    collectibles.clear();
    effects.reset();
    time = 0.0f;
    nextId = 1;
}

void World::update(float dt)
{
    time += dt;

    waves.update(dt, *this);
    for (Turtle& t : turtles) t.update(dt, *this);
    for (Villain& v : villains) v.update(dt, *this);
    for (Projectile& p : projectiles) p.update(dt);
    for (Collectible& c : collectibles) c.update(dt);
    effects.update(dt);   // before Combat, so effects spawned this frame start at full life

    Combat::resolve(*this);
    removeDead();
}

bool World::placeTurtle(TurtleKind kind, int lane)
{
    if (lane < 0 || lane >= LANE_COUNT) return false;   // TODO(C): show "select a lane first" (req 66, NF15)
    if (!economy.spend(kind)) return false;
    turtles.push_back(makeTurtle(kind, lane, nextId++));

    // ninja smoke as the turtle drops into its lane (req 68)
    const Turtle& t = turtles.back();
    effects.smokePuff(glm::vec2(t.x, t.bottom() + t.height * 0.5f), t.height * 0.5f);
    return true;
}

void World::spawnVillain(VillainKind kind, int lane)
{
    villains.push_back(makeVillain(kind, lane, nextId++));
}

void World::spawnProjectile(ProjectileKind kind, int lane, float x, bool fromTurtle)
{
    projectiles.push_back(makeProjectile(kind, lane, x, fromTurtle, nextId++));
}

void World::dropPizza(int lane, float x, int value)
{
    collectibles.push_back(makePizza(lane, x, value, nextId++));
}

bool World::villainReachedLair() const
{
    for (const Villain& v : villains)
        if (v.alive && v.left() <= LAIR_WIDTH) return true;
    return false;
}

bool World::allWavesCleared() const
{
    return waves.finished() && villains.empty();
}

template <typename T>
static void removeDeadFrom(std::vector<T>& list)
{
    list.erase(std::remove_if(list.begin(), list.end(), [](const T& e) { return !e.alive; }), list.end());
}

void World::removeDead()
{
    removeDeadFrom(turtles);
    removeDeadFrom(villains);
    removeDeadFrom(projectiles);
    removeDeadFrom(collectibles);
}
