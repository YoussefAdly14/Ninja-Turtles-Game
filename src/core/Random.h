#pragma once
#include <cstdlib>
#include <ctime>

// rand()/srand() like the learnopengl Breakout code (req 13; CLAUDE.md: no <random>).
// seed 0 = pick a new seed (clock mixed with the current rand() state, so two games started within the
// same second still differ), any other value = use exactly that one. Returns the seed in use, so it can be
// printed and later passed back in to repeat the same game (never 0).
inline unsigned int seedRandom(unsigned int seed)
{
    if (seed == 0) {
        seed = static_cast<unsigned int>(time(nullptr)) ^ static_cast<unsigned int>(rand());
        if (seed == 0) seed = 1;
    }
    srand(seed);
    return seed;
}

inline int randomInt(int count)   // 0 .. count-1
{
    return rand() % count;
}

inline float randomFloat(float min, float max)
{
    return min + (max - min) * (rand() / static_cast<float>(RAND_MAX));
}
