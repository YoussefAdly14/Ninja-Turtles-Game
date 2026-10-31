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

// The walkways themselves are part of the sewer background (Environment's ledge mesh, so manholes
// and grates can lie on them). Here: lane separators + hover/selected highlights (req 39, 52, 53).
// Everything is semi-transparent so the characters stay readable.
// Hover = faint white fill + dashed edges. Selected = pulsing green fill + solid border + a marker by the Lair.
void LaneManager::draw(float time) const
{
    const float laneWidth = WORLD_WIDTH - LAIR_WIDTH;
    const float centerX = LAIR_WIDTH + laneWidth * 0.5f;
    const glm::vec4 separator = hexColor(0x0B100E, 0.9f);
    const float pulse = 0.5f + 0.5f * glm::sin(time * 3.0f);

    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        float bottom = laneBottom(lane), top = laneTop(lane);
        glm::vec2 center(centerX, laneCenterY(lane));
        glm::vec2 size(laneWidth, LANE_HEIGHT);

        // separator: dark shadow line under each walkway's curb
        Renderer::drawLine({ LAIR_WIDTH, bottom - 1.5f }, { WORLD_WIDTH, bottom - 1.5f }, 3.0f, separator);

        if (m_selected[lane]) {
            Renderer::drawRect(center, size, withAlpha(MUTAGEN_GLOW, 0.12f + 0.06f * pulse));
            // soft inner glow along the edges, then a solid border on three sides
            Renderer::drawRect({ centerX, top - 9.0f }, { laneWidth, 14.0f }, withAlpha(MUTAGEN_GLOW, 0.10f));
            Renderer::drawRect({ centerX, bottom + 9.0f }, { laneWidth, 14.0f }, withAlpha(MUTAGEN_GLOW, 0.10f));
            Renderer::drawLine({ LAIR_WIDTH, top - 2.0f }, { WORLD_WIDTH, top - 2.0f }, 4.0f, withAlpha(MUTAGEN_GLOW, 0.9f));
            Renderer::drawLine({ LAIR_WIDTH, bottom + 2.0f }, { WORLD_WIDTH, bottom + 2.0f }, 4.0f, withAlpha(MUTAGEN_GLOW, 0.9f));
            Renderer::drawLine({ WORLD_WIDTH - 2.0f, bottom }, { WORLD_WIDTH - 2.0f, top }, 4.0f, withAlpha(MUTAGEN_GLOW, 0.9f));

            // marker by the Lair: dark tab with a glowing ">>" pointing into the lane
            float my = laneCenterY(lane), mx = LAIR_WIDTH + 18.0f;
            Renderer::drawRect({ mx, my }, { 34.0f, 70.0f }, withAlpha(MUTAGEN_GLOW, 0.9f));
            Renderer::drawRect({ mx, my }, { 28.0f, 64.0f }, hexColor(0x0F1A0B, 0.92f));
            for (int k = 0; k < 2; ++k) {
                float cx = mx - 5.0f + k * 9.0f;
                glm::vec4 c = withAlpha(MUTAGEN_GLOW, k == 0 ? 0.55f + 0.45f * pulse : 1.0f - 0.45f * pulse);
                Renderer::drawRect({ cx, my + 6.0f }, { 4.0f, 16.0f }, c, 40.0f);
                Renderer::drawRect({ cx, my - 6.0f }, { 4.0f, 16.0f }, c, -40.0f);
            }
            Renderer::drawCircle({ mx, my + 24.0f }, 3.5f, withAlpha(MUTAGEN_GLOW, 0.6f + 0.4f * pulse));
            Renderer::drawCircle({ mx, my - 24.0f }, 3.5f, withAlpha(MUTAGEN_GLOW, 0.6f + 0.4f * pulse));
        }
        if (lane == hoveredLane) {
            Renderer::drawRect(center, size, withAlpha(WHITE, 0.10f));
            for (float x = LAIR_WIDTH + 6.0f; x < WORLD_WIDTH; x += 48.0f) {
                Renderer::drawRect({ x + 15.0f, top - 3.0f }, { 30.0f, 3.0f }, withAlpha(WHITE, 0.6f));
                Renderer::drawRect({ x + 15.0f, bottom + 3.0f }, { 30.0f, 3.0f }, withAlpha(WHITE, 0.6f));
            }
        }
    }
}
