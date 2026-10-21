#pragma once
#include <glm/glm.hpp>
#include "Config.h"

// ---- lane geometry (only depends on Config.h). Lane 0 is the top lane. ----
float laneBottom(int lane);
float laneTop(int lane);
float laneCenterY(int lane);
float laneFloorY(int lane);           // where characters stand
int   laneAt(glm::vec2 worldPos);     // -1 outside the playfield (HUD, Lair, water, letterbox)

// ---- hover + selection (req 52-55) ----
class LaneManager
{
public:
    int hoveredLane = -1;
    int lastSelectedLane = -1;        // where a bought turtle goes (OQ2)

    void reset();
    void updateHover(glm::vec2 mouseWorld);
    void toggle(int lane);
    bool isSelected(int lane) const;

    void draw(float time) const;      // walkways, separators, hover/selected highlights

private:
    bool m_selected[LANE_COUNT] = {};
};
