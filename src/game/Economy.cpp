#include "game/Economy.h"
#include "Config.h"

int turtleCost(TurtleKind kind)
{
    switch (kind) {
    case TurtleKind::Leonardo:  return LEONARDO_COST;
    case TurtleKind::Raphael:   return RAPHAEL_COST;
    case TurtleKind::Donatello: return DONATELLO_COST;
    default:                    return 0;
    }
}

int pizzaValue(VillainKind kind, int waveNumber)
{
    int base = PIZZA_VALUE;
    if (kind == VillainKind::Bebop) base = BEBOP_PIZZA_VALUE;
    else if (kind == VillainKind::Rocksteady) base = ROCKSTEADY_PIZZA_VALUE;
    int wavesAfterFirst = waveNumber > 1 ? waveNumber - 1 : 0;
    return base + PIZZA_WAVE_BONUS * wavesAfterFirst;
}

void Economy::reset()
{
    coins = START_COINS;
}

bool Economy::isUnlocked(TurtleKind kind) const
{
    return coins >= turtleCost(kind);
}

bool Economy::spend(TurtleKind kind)
{
    if (!isUnlocked(kind)) return false;
    coins -= turtleCost(kind);
    return true;
}

// TODO(C): pulse the HUD counter when coins change (req 60)
void Economy::add(int amount)
{
    coins += amount;
}
