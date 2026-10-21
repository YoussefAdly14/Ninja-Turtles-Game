#pragma once
#include <vector>
#include "game/Types.h"

struct World;

// Three waves of 3 / 5 / 10 villains in random lanes at random intervals (req 98-102).
// A wave ends when its last villain appears; the next starts only once the screen is clear.
class WaveManager
{
public:
    void reset();
    void update(float dt, World& world);

    int  waveNumber() const { return m_wave + 1; }    // 1-based, for the HUD
    bool isBetweenWaves() const;                      // for a "Wave 2/3" banner (req 103)
    bool finished() const { return m_finished; }      // every wave spawned AND cleared -> win

private:
    int   m_wave = 0;
    bool  m_finished = false;
    float m_spawnTimer = 0.0f;
    std::vector<VillainKind> m_queue;    // villains of the current wave still to spawn

    void buildWave(int wave);
};
