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
