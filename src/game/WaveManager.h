#pragma once
#include <vector>
#include "game/Types.h"
#include "Config.h"

struct World;

// Three waves of 3 / 5 / 10 villains in random lanes at random intervals (req 98-102).
// A wave ends when its last villain appears; the next starts only once the screen is clear.
// The whole schedule (kind, lane, gap of every villain of every wave) is drawn from rand() in reset(),
// so a game's waves depend only on the seed and nothing that happens during play can change them (req 13).
class WaveManager
{
public:
    void reset();                                     // call right after the seed is set
    void update(float dt, World& world);

    int  waveNumber() const { return m_wave + 1; }    // 1-based, for the HUD
    bool isBetweenWaves() const;                      // for a "Wave 2/3" banner (req 103)
    bool finished() const { return m_finished; }      // every wave spawned AND cleared -> win
    void skipRestOfWave() { m_next = static_cast<int>(m_waves[m_wave].size()); }   // dev aid (req 17): no more spawns this wave

private:
    struct Spawn
    {
        VillainKind kind;
        int lane;
        float delay;    // seconds to wait after the previous spawn (the first one: after the game / wave break starts)
    };

    int   m_wave = 0;
    bool  m_finished = false;
    float m_spawnTimer = 0.0f;
    int   m_next = 0;                          // villains of the current wave already spawned
    std::vector<Spawn> m_waves[WAVE_COUNT];    // the schedule of every wave, filled once by reset()

    void buildWave(int wave);
};
