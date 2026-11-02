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
    for (int lane = 0; lane < LANE_COUNT; ++lane) { bonusPizzaWave[lane] = 0; bonusPizzaShown[lane] = 0; }
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
    spawnBonusPizzas();
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

void World::spawnBonusPizzas()
{
    const int wave = waves.waveNumber();
    if (wave < BONUS_PIZZA_FIRST_WAVE || wave > BONUS_PIZZA_LAST_WAVE) return;

    for (int lane = 0; lane < LANE_COUNT; ++lane)
    {
        const Turtle* leftmost = nullptr;
        float frontX = 0.0f;   // furthest-advanced turtle centre
        bool hasVillain = false;
        for (const Turtle& t : turtles)
        {
            if (!t.alive || t.lane != lane) continue;
            if (!leftmost || t.x < leftmost->x) leftmost = &t;
            if (t.x > frontX) frontX = t.x;
        }
        for (const Villain& v : villains)
            if (v.alive && v.lane == lane && v.left() < WORLD_WIDTH) hasVillain = true;

        if (bonusPizzaWave[lane] != wave)
        {
            if (!leftmost || !hasVillain) continue;   // start only with a turtle AND a villain on screen
            bonusPizzaWave[lane] = wave;
            bonusPizzaShown[lane] = 0;
        }
        if (!leftmost) continue;

        while (bonusPizzaShown[lane] < BONUS_PIZZA_SPOT_COUNT)
        {
            const int i = bonusPizzaShown[lane];
            if (i > 0 && frontX <= BONUS_PIZZA_COLUMNS_X[i - 1]) break;   // no turtle has passed this column yet
            if (BONUS_PIZZA_SPOTS_X[i] > leftmost->right())               // skip spots behind the leftmost turtle
                dropPizza(lane, BONUS_PIZZA_SPOTS_X[i], BONUS_PIZZA_VALUE);
            ++bonusPizzaShown[lane];
        }
    }
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
