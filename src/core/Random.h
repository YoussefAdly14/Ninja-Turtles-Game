#pragma once
#include <cstdlib>
#include <ctime>

// rand()/srand() like the learnopengl Breakout code (req 13; CLAUDE.md: no <random>).
// seed 0 = different game every run, any other value = same lanes/order every run (for the video).
inline void seedRandom(unsigned int seed)
{
    srand(seed != 0 ? seed : static_cast<unsigned int>(time(nullptr)));
}

inline int randomInt(int count)   // 0 .. count-1
{
    return rand() % count;
}

inline float randomFloat(float min, float max)
{
    return min + (max - min) * (rand() / static_cast<float>(RAND_MAX));
}
