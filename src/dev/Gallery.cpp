#include "dev/Gallery.h"
#include "models/TurtleModels.h"
#include "models/VillainModels.h"
#include "models/PropModels.h"
#include "models/LairModel.h"
#include "scene/Environment.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "ui/Text.h"
#include "Config.h"
#include <cmath>
#include <cstring>

enum GalleryPage { PAGE_TURTLES, PAGE_VILLAINS, PAGE_PROPS };

const int POSE_COUNT = 7;
const char* POSE_NAMES[POSE_COUNT] = { "Idle", "Walk", "Attack", "Duck", "Jump", "Hurt", "Locked" };
const float CHARACTER_SCALE = 1.3f;
const float ROW_HEIGHT = WORLD_HEIGHT / 3.0f;

int Gallery::pageFromName(const char* name)
{
    if (strcmp(name, "turtles") == 0) return PAGE_TURTLES;
    if (strcmp(name, "villains") == 0) return PAGE_VILLAINS;
    if (strcmp(name, "props") == 0) return PAGE_PROPS;
    return -1;
}

// pose column -> Pose + extra lift for the jump
static Pose makePose(int column, float time, float& lift)
{
    Pose p;
    p.time = time;
    lift = 0.0f;
    switch (column) {
    case 1: p.walkCycle = time * 8.0f; break;
    case 2: p.attack = fmodf(time, 1.0f); break;
    case 3: p.ducking = true; break;
    case 4: p.inAir = true; lift = 40.0f; break;
    case 5: p.hitFlash = 0.5f + 0.5f * sinf(time * 6.0f); break;
    default: break;
    }
    return p;
}

static void drawCharacterGrid(bool turtles, float time)
{
    float columnWidth = WORLD_WIDTH / POSE_COUNT;
    for (int row = 0; row < 3; ++row) {
        float rowBottom = WORLD_HEIGHT - (row + 1) * ROW_HEIGHT;
        Renderer::drawLine({ 0.0f, rowBottom }, { WORLD_WIDTH, rowBottom }, 2.0f, withAlpha(BLACK, 0.5f));

        for (int col = 0; col < POSE_COUNT; ++col) {
            float lift;
            Pose pose = makePose(col, time, lift);
            float x = columnWidth * (col + 0.5f);
            glm::mat4 base = translate2D(glm::mat4(1.0f), x, rowBottom + 40.0f + lift);
            base = scale2D(base, CHARACTER_SCALE, CHARACTER_SCALE);

            // floor line under the feet
            Renderer::drawLine({ x - columnWidth * 0.4f, rowBottom + 40.0f }, { x + columnWidth * 0.4f, rowBottom + 40.0f }, 2.0f, withAlpha(WHITE, 0.3f));

            if (col == 5) Renderer::setDamageTint(0.6f);
            if (col == 6) Renderer::setGrayscale(1.0f);
            if (turtles) TurtleModels::draw((TurtleKind)row, base, pose);
            else         VillainModels::draw((VillainKind)row, base, pose);
            Renderer::setDamageTint(0.0f);
            Renderer::setGrayscale(0.0f);

            if (row == 0) Text::drawCentered(POSE_NAMES[col], x, WORLD_HEIGHT - 30.0f, 0.5f, WHITE);
        }
    }
}

static void drawProps(float time)
{
    Environment::drawBackground(time);
    Environment::drawProps(time);
    LairModel::draw(time);

    const float y = WORLD_HEIGHT * 0.5f;
    const float scale = 3.0f;
    glm::mat4 m = glm::mat4(1.0f);
    PropModels::drawPizza(scale2D(translate2D(m, 450.0f, y), scale, scale), time);
    PropModels::drawSai(scale2D(translate2D(m, 750.0f, y), scale, scale), time * 720.0f);
    PropModels::drawShuriken(scale2D(translate2D(m, 1050.0f, y), scale, scale), time * 720.0f);
    PropModels::drawBlasterBolt(scale2D(translate2D(m, 1350.0f, y), scale, scale), time);
}

void Gallery::draw(int page, float time)
{
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, WORLD_HEIGHT * 0.5f }, { WORLD_WIDTH, WORLD_HEIGHT }, SEWER_WALL);
    switch (page) {
    case PAGE_TURTLES:  drawCharacterGrid(true, time);  break;
    case PAGE_VILLAINS: drawCharacterGrid(false, time); break;
    case PAGE_PROPS:    drawProps(time);                break;
    default: break;
    }
}
