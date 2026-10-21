#include "game/LaneManager.h"
#include "core/Renderer.h"
#include "core/Colors.h"

float laneBottom(int lane)  { return PLAYFIELD_TOP - (lane + 1) * LANE_HEIGHT; }
float laneTop(int lane)     { return PLAYFIELD_TOP - lane * LANE_HEIGHT; }
float laneCenterY(int lane) { return laneBottom(lane) + LANE_HEIGHT * 0.5f; }
float laneFloorY(int lane)  { return laneBottom(lane) + LANE_FLOOR_OFFSET; }

int laneAt(glm::vec2 p)
{
    if (p.x < LAIR_WIDTH || p.x > WORLD_WIDTH) return -1;
    if (p.y < PLAYFIELD_BOTTOM || p.y >= PLAYFIELD_TOP) return -1;
    int lane = static_cast<int>((PLAYFIELD_TOP - p.y) / LANE_HEIGHT);
    return (lane >= 0 && lane < LANE_COUNT) ? lane : -1;
}

void LaneManager::reset()
{
    hoveredLane = -1;
    lastSelectedLane = -1;
    for (bool& s : m_selected) s = false;
}

void LaneManager::updateHover(glm::vec2 mouseWorld)
{
    hoveredLane = laneAt(mouseWorld);
}

void LaneManager::toggle(int lane)
{
    if (lane < 0 || lane >= LANE_COUNT) return;
    m_selected[lane] = !m_selected[lane];

    if (m_selected[lane]) {
        lastSelectedLane = lane;
    }
    else if (lastSelectedLane == lane) {
        // fall back to another lane that is still selected
        lastSelectedLane = -1;
        for (int i = 0; i < LANE_COUNT; ++i)
            if (m_selected[i]) lastSelectedLane = i;
    }
}

bool LaneManager::isSelected(int lane) const
{
    return lane >= 0 && lane < LANE_COUNT && m_selected[lane];
}

// TODO(C): final look of hover vs selected (req 52, 53: brighter fill + border + marker by the Lair)
// TODO(D): real sewer walkway art (req 39, 111)
void LaneManager::draw(float /*time*/) const
{
    const float laneWidth = WORLD_WIDTH - LAIR_WIDTH;
    const float centerX = LAIR_WIDTH + laneWidth * 0.5f;

    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        // walkway strip + separator line
        Renderer::drawRect({ centerX, laneBottom(lane) + LANE_FLOOR_OFFSET * 0.5f },
                           { laneWidth, LANE_FLOOR_OFFSET }, hexColor(0x2A3632));
        Renderer::drawLine({ LAIR_WIDTH, laneBottom(lane) }, { WORLD_WIDTH, laneBottom(lane) }, 3.0f, hexColor(0x1C2522));

        // highlights: semi-transparent so characters stay visible
        glm::vec2 center(centerX, laneCenterY(lane));
        glm::vec2 size(laneWidth, LANE_HEIGHT);
        if (m_selected[lane]) {
            Renderer::drawRect(center, size, withAlpha(MUTAGEN_GLOW, 0.18f));
            Renderer::drawLine({ LAIR_WIDTH, laneTop(lane) - 2.0f }, { WORLD_WIDTH, laneTop(lane) - 2.0f }, 4.0f, withAlpha(MUTAGEN_GLOW, 0.8f));
            Renderer::drawLine({ LAIR_WIDTH, laneBottom(lane) + 2.0f }, { WORLD_WIDTH, laneBottom(lane) + 2.0f }, 4.0f, withAlpha(MUTAGEN_GLOW, 0.8f));
        }
        if (lane == hoveredLane)
            Renderer::drawRect(center, size, withAlpha(WHITE, 0.10f));
    }
}
