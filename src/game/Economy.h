#pragma once
#include "game/Types.h"

int turtleCost(TurtleKind kind);
int turtleUnlockWave(TurtleKind kind);              // first wave (1-based) in which the card can be used
int pizzaValue(VillainKind kind, int waveNumber);   // what a defeated villain drops (waveNumber is 1-based)

// Pizza (coins) + card lock state (req 59, 64, 65, 67).
class Economy
{
public:
    int coins = 0;

    void reset();
    // card unlocked = its wave has started AND coins >= cost, re-checked every frame (waveNumber is 1-based)
    bool isUnlocked(TurtleKind kind, int waveNumber) const;
    bool spend(TurtleKind kind, int waveNumber);   // atomic: check, deduct; false if locked or not affordable
    void add(int amount);
};
