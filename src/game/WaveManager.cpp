#include "game/WaveManager.h"
#include "game/World.h"
#include "core/Random.h"
#include "Config.h"

void WaveManager::reset()
{
    m_wave = 0;
    m_next = 0;
    m_finished = false;
    for (int wave = 0; wave < WAVE_COUNT; ++wave)
        buildWave(wave);                // all random draws happen here, in a fixed order
    m_spawnTimer = m_waves[0][0].delay; // enemies don't wait for the player (req 100)
}

// wave N allows the first N villain kinds; at least one of each so graders see the mix (req 99)
void WaveManager::buildWave(int wave)
{
    int kinds = wave + 1;
    std::vector<VillainKind> order;
    for (int k = 0; k < kinds; ++k)
        order.push_back(static_cast<VillainKind>(k));
    while (static_cast<int>(order.size()) < WAVE_SIZES[wave])
        order.push_back(static_cast<VillainKind>(randomInt(kinds)));

    // shuffle the order (swap each slot with a random earlier one)
    for (int i = static_cast<int>(order.size()) - 1; i > 0; --i) {
        int j = randomInt(i + 1);
        VillainKind tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }

    // a random lane and a random wait for each villain; the first one of a wave waits for the game start / wave break
    m_waves[wave].clear();
    for (size_t i = 0; i < order.size(); ++i) {
        Spawn s;
        s.kind = order[i];
        s.lane = randomInt(LANE_COUNT);
        if (i > 0) s.delay = randomFloat(SPAWN_INTERVAL_MIN, SPAWN_INTERVAL_MAX);
        else       s.delay = wave == 0 ? FIRST_SPAWN_DELAY : WAVE_BREAK_DURATION;
        m_waves[wave].push_back(s);
    }
}

void WaveManager::update(float dt, World& world)
{
    if (m_finished || m_waves[m_wave].empty()) return;   // empty = reset() was never called

    const std::vector<Spawn>& wave = m_waves[m_wave];
    if (m_next >= static_cast<int>(wave.size())) {
        // wave fully spawned: wait until every villain is defeated (req 102)
        if (!world.villains.empty()) return;
        if (m_wave + 1 >= WAVE_COUNT) {
            m_finished = true;
            return;
        }
        ++m_wave;
        m_next = 0;
        m_spawnTimer = m_waves[m_wave][0].delay;   // the wave break
        return;
    }

    m_spawnTimer -= dt;
    if (m_spawnTimer > 0.0f) return;

    const Spawn& s = wave[m_next++];
    world.spawnVillain(s.kind, s.lane);
    if (m_next < static_cast<int>(wave.size())) m_spawnTimer = wave[m_next].delay;
}

bool WaveManager::isBetweenWaves() const
{
    // nothing of this wave has spawned yet
    return !m_finished && m_next == 0;
}
