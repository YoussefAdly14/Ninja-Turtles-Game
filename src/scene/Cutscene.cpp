#include "scene/Cutscene.h"
#include "scene/Timeline.h"
#include "scene/Environment.h"
#include "models/TurtleModels.h"
#include "models/VillainModels.h"
#include "models/LairModel.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "ui/Text.h"
#include "Config.h"

void Cutscene::start()
{
    m_time = 0.0f;
}

void Cutscene::update(float dt)
{
    m_time += dt;
}

bool Cutscene::finished() const
{
    return m_time >= CUTSCENE_DURATION;
}

// TODO(D): PLACEHOLDER showing the pieces: a camera pan (view matrix), two actors with keyframed
// positions and poses, a subtitle, and a fade out. Replace with the 16 s storyboard.
void Cutscene::draw() const
{
    float t = m_time;

    // camera starts zoomed in near the Lair and pulls back to the full view during the first 3 s
    // view = move focus to the screen centre, zoom around it (learnopengl Camera / Lab 4 view matrix idea)
    float k = easeInOut(segment(t, 0.0f, 3.0f));
    float zoom = lerp(1.5f, 1.0f, k);
    glm::vec2 focus(lerp(560.0f, WORLD_WIDTH * 0.5f, k), WORLD_HEIGHT * 0.5f);
    glm::mat4 view = translate2D(glm::mat4(1.0f), WORLD_WIDTH * 0.5f, WORLD_HEIGHT * 0.5f);
    view = scale2D(view, zoom, zoom);
    view = translate2D(view, -focus.x, -focus.y);
    Renderer::setView(view);

    Environment::drawBackground(t);
    LairModel::draw(t);

    float floorY = PLAYFIELD_BOTTOM + 200.0f;

    // Leonardo walks out of the Lair, the Foot Soldier walks in, they clash around 8 s
    Pose leoPose;
    leoPose.time = t;
    leoPose.walkCycle = t * 8.0f;
    leoPose.attack = segment(t, 8.0f, 8.5f);
    float leoX = lerp(LAIR_WIDTH + 40.0f, 700.0f, easeInOut(segment(t, 3.0f, 8.0f)));
    TurtleModels::drawLeonardo(translate2D(glm::mat4(1.0f), leoX, floorY), leoPose);

    Pose footPose;
    footPose.time = t;
    footPose.walkCycle = t * 8.0f;
    float footX = lerp(1700.0f, 820.0f, easeInOut(segment(t, 5.0f, 8.0f)));
    VillainModels::drawFootSoldier(translate2D(glm::mat4(1.0f), footX, floorY), footPose);

    Renderer::setView(glm::mat4(1.0f));   // subtitles and fades don't move with the camera

    if (t > 3.0f && t < 7.0f)
        Text::drawCentered("Last canister of mutagen. In the wrong hands, it makes monsters.",
                           WORLD_WIDTH * 0.5f, 90.0f, 0.6f, WHITE);

    // fade to black in the last second
    float fade = segment(t, CUTSCENE_DURATION - 1.0f, CUTSCENE_DURATION);
    if (fade > 0.0f)
        Renderer::drawRect({ WORLD_WIDTH * 0.5f, WORLD_HEIGHT * 0.5f }, { WORLD_WIDTH, WORLD_HEIGHT }, withAlpha(BLACK, fade));
}
