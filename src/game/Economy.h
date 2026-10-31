#pragma once
#include "game/Types.h"

int turtleCost(TurtleKind kind);
int pizzaValue(VillainKind kind, int waveNumber);   // what a defeated villain drops (waveNumber is 1-based)

// Pizza (coins) + card lock state (req 59, 64, 65, 67).
class Economy
{
public:
    int coins = 0;

    void reset();
    bool isUnlocked(TurtleKind kind) const;   // card unlocked = coins >= cost, re-checked every frame
    bool spend(TurtleKind kind);              // atomic: check, deduct; false if not affordable
    void add(int amount);
};
