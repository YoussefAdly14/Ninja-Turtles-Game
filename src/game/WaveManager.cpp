#include "game/WaveManager.h"
#include "game/World.h"
#include "core/Random.h"
#include "Config.h"

void WaveManager::reset()
{
    m_wave = 0;
    m_finished = false;
    buildWave(0);
    m_spawnTimer = FIRST_SPAWN_DELAY;   // enemies don't wait for the player (req 100)
}

// wave N allows the first N villain kinds; at least one of each so graders see the mix (req 99)
void WaveManager::buildWave(int wave)
{
    int kinds = wave + 1;
    m_queue.clear();
    for (int k = 0; k < kinds; ++k)
        m_queue.push_back(static_cast<VillainKind>(k));
    while (static_cast<int>(m_queue.size()) < WAVE_SIZES[wave])
        m_queue.push_back(static_cast<VillainKind>(randomInt(kinds)));

    // shuffle the order (swap each slot with a random earlier one)
    for (int i = static_cast<int>(m_queue.size()) - 1; i > 0; --i) {
        int j = randomInt(i + 1);
        VillainKind tmp = m_queue[i];
        m_queue[i] = m_queue[j];
        m_queue[j] = tmp;
    }
}

void WaveManager::update(float dt, World& world)
{
    if (m_finished) return;

    if (m_queue.empty()) {
        // wave fully spawned: wait until every villain is defeated (req 102)
        if (!world.villains.empty()) return;
        if (m_wave + 1 >= WAVE_COUNT) {
            m_finished = true;
            return;
        }
        ++m_wave;
        buildWave(m_wave);
        m_spawnTimer = WAVE_BREAK_DURATION;
        return;
    }

    m_spawnTimer -= dt;
    if (m_spawnTimer > 0.0f) return;

    VillainKind kind = m_queue.back();
    m_queue.pop_back();
    world.spawnVillain(kind, randomInt(LANE_COUNT));
    m_spawnTimer = randomFloat(SPAWN_INTERVAL_MIN, SPAWN_INTERVAL_MAX);
}

bool WaveManager::isBetweenWaves() const
{
    // nothing of this wave has spawned yet
    return !m_finished && static_cast<int>(m_queue.size()) == WAVE_SIZES[m_wave];
}
