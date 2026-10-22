#include "ui/Menu.h"
#include "ui/Text.h"
#include "ui/UiStyle.h"
#include "core/Input.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "models/TurtleModels.h"
#include "models/VillainModels.h"
#include "audio/Audio.h"
#include "Config.h"
#include <cmath>
#include <string>
#include <utility>

// ---- shared column layout (pause / win / lose / sound) ----
const glm::vec2 BUTTON_SIZE(340.0f, 70.0f);
const float BUTTON_GAP = 95.0f;
const float FIRST_BUTTON_Y = 470.0f;
const float TITLE_Y = 640.0f;
const float TITLE_SCALE = 1.5f;
const float SUBTITLE_Y = 588.0f;
const float PANEL_WIDTH = 520.0f;
const float PANEL_ABOVE_TITLE = 85.0f;
const float PANEL_BELOW_BUTTONS = 35.0f;
const float OVERLAY_DIM = 0.6f;

// ---- main menu ----
const float MAIN_TITLE_Y = 712.0f;
const float MAIN_TITLE_SCALE = 2.7f;
const float RIBBON_Y = 640.0f;
const float MAIN_FIRST_BUTTON_Y = 525.0f;
const float MAIN_BUTTON_GAP = 92.0f;
const float LEDGE_Y = 150.0f;                 // the stone ledge the figures stand on
const float MENU_TURTLE_SCALE = 1.6f;
const float MENU_VILLAIN_SCALE = 1.45f;
const float FIGURE_ATTACK_PERIOD = 3.5f;      // each figure swings/throws once every few seconds

// ---- sound settings ----
const float SOUND_FIRST_BUTTON_Y = 430.0f;
const int   VOLUME_SEGMENTS = 10;
const float VOLUME_SEGMENT_W = 26.0f;
const float VOLUME_SEGMENT_GAP = 5.0f;
const float VOLUME_METER_Y = 505.0f;          // bottom of the meter bars

// ---- pause: controls legend panel (req 28) ----
const float PAUSE_COLUMN_X = 540.0f;
const float LEGEND_X = 1120.0f;
const float LEGEND_WIDTH = 540.0f;
const float LEGEND_ROW_GAP = 40.0f;

// ================= small pieces of art =================

// soft oval shadow so the figures stand on something
static void drawShadow(float x, float y, float width)
{
    Renderer::draw(Shape::Circle, scale2D(translate2D(glm::mat4(1.0f), x, y), width, width * 0.22f), withAlpha(BLACK, 0.4f));
}

// idle pose + one attack every FIGURE_ATTACK_PERIOD seconds (offset so they don't move in sync)
static Pose figurePose(float t, float offset, bool attacks)
{
    Pose p;
    p.time = t + offset;
    if (attacks) {
        float cycle = std::fmod(t + offset, FIGURE_ATTACK_PERIOD);
        if (cycle < ATTACK_ANIM_DURATION) p.attack = cycle / ATTACK_ANIM_DURATION;
    }
    return p;
}

static void drawTurtleFigure(TurtleKind kind, float x, float floorY, float scale, const Pose& pose, float lift = 0.0f)
{
    drawShadow(x, floorY, TURTLE_WIDTH * scale * (lift > 0.0f ? 0.8f : 1.1f));
    glm::mat4 base = translate2D(glm::mat4(1.0f), x, floorY + lift);
    TurtleModels::draw(kind, scale2D(base, scale, scale), pose);
}

static void drawVillainFigure(VillainKind kind, float x, float floorY, float scale, const Pose& pose)
{
    float width = kind == VillainKind::Rocksteady ? ROCKSTEADY_WIDTH : (kind == VillainKind::Bebop ? BEBOP_WIDTH : FOOT_WIDTH);
    drawShadow(x, floorY, width * scale * 1.1f);
    glm::mat4 base = translate2D(glm::mat4(1.0f), x, floorY);
    VillainModels::draw(kind, scale2D(base, scale, scale), pose);
}

// the last mutagen canister: what the turtles defend (story beat on the menu)
static void drawCanister(float x, float bottomY, float t)
{
    float glow = 0.5f + 0.5f * std::sin(t * 3.0f);
    Renderer::drawCircle({ x, bottomY + 55.0f }, 70.0f + 8.0f * glow, withAlpha(MUTAGEN_GLOW, 0.10f + 0.08f * glow));
    Renderer::drawCircle({ x, bottomY + 55.0f }, 48.0f, withAlpha(MUTAGEN_GLOW, 0.12f));

    // pedestal
    Renderer::drawRect({ x, bottomY + 6.0f }, { 84.0f, 16.0f }, UI_FRAME);
    Renderer::drawRect({ x, bottomY + 6.0f }, { 78.0f, 10.0f }, hexColor(0x4A4F4C));
    // glass tube with the glowing liquid
    Renderer::drawRoundedRect({ x, bottomY + 55.0f }, { 52.0f, 82.0f }, 10.0f, UI_FRAME);
    Renderer::drawRoundedRect({ x, bottomY + 55.0f }, { 44.0f, 76.0f }, 8.0f, hexColor(0x1C3A14));
    Renderer::drawRect({ x, bottomY + 48.0f }, { 38.0f, 56.0f }, hexColor(0x4FC400));
    Renderer::drawRect({ x, bottomY + 76.0f - 2.0f * glow }, { 38.0f, 4.0f }, MUTAGEN_GLOW);
    for (int i = 0; i < 3; ++i) {   // bubbles rising through the liquid
        float rise = std::fmod(t * 18.0f + i * 19.0f, 52.0f);
        Renderer::drawCircle({ x - 10.0f + i * 10.0f, bottomY + 22.0f + rise }, 3.0f, withAlpha(WHITE, 0.6f));
    }
    Renderer::drawRect({ x - 13.0f, bottomY + 55.0f }, { 5.0f, 62.0f }, withAlpha(WHITE, 0.25f));   // glass shine
    // metal caps
    Renderer::drawRect({ x, bottomY + 18.0f }, { 58.0f, 12.0f }, UI_FRAME);
    Renderer::drawRect({ x, bottomY + 18.0f }, { 54.0f, 8.0f }, hexColor(0x8C9490));
    Renderer::drawRect({ x, bottomY + 94.0f }, { 58.0f, 12.0f }, UI_FRAME);
    Renderer::drawRect({ x, bottomY + 94.0f }, { 54.0f, 8.0f }, hexColor(0x8C9490));
    Renderer::drawRect({ x, bottomY + 103.0f }, { 20.0f, 8.0f }, hexColor(0x8C9490));
}

// red cloth ribbon (like a ninja mask band) behind the subtitle
static void drawRibbon(const std::string& text, float centerX, float y)
{
    float scale = 1.0f;
    float w = Text::width(text, scale) + 70.0f;
    const float h = 52.0f;
    glm::vec4 cloth = RAPHAEL_MASK;
    glm::vec4 fold = hexColor(0x7E1519);
    // knotted tails hanging out behind both ends, pointed at the outer end
    for (int side = -1; side <= 1; side += 2) {
        float tx = centerX + side * (w * 0.5f + 18.0f);
        glm::mat4 tip = rotate2D(translate2D(glm::mat4(1.0f), tx + side * 44.0f, y - 12.0f), side * -90.0f);
        Renderer::draw(Shape::Triangle, scale2D(tip, h + 6.0f, 34.0f), UI_FRAME);
        Renderer::draw(Shape::Triangle, scale2D(tip, h - 2.0f, 28.0f), fold);
        Renderer::drawRect({ tx, y - 12.0f }, { 70.0f, h + 6.0f }, UI_FRAME);
        Renderer::drawRect({ tx, y - 12.0f }, { 64.0f, h }, fold);
    }
    Renderer::drawRect({ centerX, y }, { w + 6.0f, h + 6.0f }, UI_FRAME);
    Renderer::drawRect({ centerX, y }, { w, h }, cloth);
    Renderer::drawRect({ centerX, y + h * 0.5f - 6.0f }, { w, 4.0f }, withAlpha(WHITE, 0.2f));
    UiStyle::drawOutlinedText(text, centerX, y - 16.0f, scale, UI_TEXT, 2.5f);
}

// ================= per-screen artwork =================

static void drawMainArt(float t)
{
    // darken the sewer a little so the title pops
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, WORLD_HEIGHT * 0.5f }, { WORLD_WIDTH, WORLD_HEIGHT }, withAlpha(BLACK, 0.25f));

    // stone ledge across the bottom with brick joints
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, LEDGE_Y - 40.0f }, { WORLD_WIDTH, 80.0f }, hexColor(0x2C3833));
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, LEDGE_Y - 3.0f }, { WORLD_WIDTH, 6.0f }, hexColor(0x52625B));
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, LEDGE_Y - 40.0f }, { WORLD_WIDTH, 4.0f }, hexColor(0x222C28));
    for (int i = 0; i < 16; ++i) {   // brick joints, the lower row shifted by half a brick
        Renderer::drawRect({ i * 100.0f + 50.0f, LEDGE_Y - 22.0f }, { 4.0f, 32.0f }, hexColor(0x222C28));
        Renderer::drawRect({ i * 100.0f,         LEDGE_Y - 60.0f }, { 4.0f, 36.0f }, hexColor(0x222C28));
    }

    drawCanister(WORLD_WIDTH * 0.5f, LEDGE_Y, t);

    // the heroes on the left facing right, the villains on the right facing left: the conflict in one picture
    drawTurtleFigure(TurtleKind::Raphael,   140.0f, LEDGE_Y, MENU_TURTLE_SCALE, figurePose(t, 1.2f, true));
    drawTurtleFigure(TurtleKind::Donatello, 455.0f, LEDGE_Y, MENU_TURTLE_SCALE, figurePose(t, 0.5f, false));
    drawTurtleFigure(TurtleKind::Leonardo,  300.0f, LEDGE_Y, MENU_TURTLE_SCALE, figurePose(t, 0.0f, true));
    drawVillainFigure(VillainKind::FootSoldier, 1150.0f, LEDGE_Y, MENU_VILLAIN_SCALE, figurePose(t, 2.3f, true));
    drawVillainFigure(VillainKind::Rocksteady,  1470.0f, LEDGE_Y, MENU_VILLAIN_SCALE, figurePose(t, 0.8f, true));
    drawVillainFigure(VillainKind::Bebop,       1310.0f, LEDGE_Y, MENU_VILLAIN_SCALE, figurePose(t, 1.7f, true));

    // title: big outlined comic letters + the red ribbon
    float bob = 4.0f * std::sin(t * 2.0f);
    UiStyle::drawOutlinedText("SHELL SHOCK", WORLD_WIDTH * 0.5f, MAIN_TITLE_Y + bob, MAIN_TITLE_SCALE, MUTAGEN_GLOW, 5.0f);
    drawRibbon("DEFEND THE LAIR", WORLD_WIDTH * 0.5f, RIBBON_Y);

    Text::drawCentered("DMET 502 COMPUTER GRAPHICS  -  GUC WINTER 2026", WORLD_WIDTH * 0.5f, 18.0f, 0.4f, UI_TEXT_DIM);
}

// current volume as a rising meter + %, or MUTED (req 127)
static void drawVolumeMeter(float centerX)
{
    float volume = Audio::volume();
    bool muted = Audio::isMuted();
    int filled = static_cast<int>(volume * VOLUME_SEGMENTS + 0.5f);

    Text::drawCentered("MASTER VOLUME", centerX, 568.0f, 0.6f, UI_TEXT_DIM);

    float meterW = VOLUME_SEGMENTS * VOLUME_SEGMENT_W + (VOLUME_SEGMENTS - 1) * VOLUME_SEGMENT_GAP;
    float left = centerX - meterW * 0.5f - 5.0f;

    // speaker icon left of the meter: box + cone, sound waves (or a red cross when muted)
    float sx = left - 70.0f, sy = VOLUME_METER_Y + 20.0f;
    Renderer::drawRect({ sx - 8.0f, sy }, { 14.0f, 18.0f }, UI_TEXT);
    glm::mat4 cone = rotate2D(translate2D(glm::mat4(1.0f), sx + 4.0f, sy), 90.0f);
    Renderer::draw(Shape::Triangle, scale2D(cone, 34.0f, 22.0f), UI_TEXT);
    if (muted) {
        Renderer::drawLine({ sx + 22.0f, sy - 12.0f }, { sx + 42.0f, sy + 12.0f }, 5.0f, UI_WARNING);
        Renderer::drawLine({ sx + 22.0f, sy + 12.0f }, { sx + 42.0f, sy - 12.0f }, 5.0f, UI_WARNING);
    }
    else {
        for (int k = 1; k <= 2; ++k) {   // each wave = an arc of 3 short lines
            float r = 6.0f + 10.0f * k;
            for (int j = 0; j < 3; ++j) {
                float a0 = glm::radians(-45.0f + 30.0f * j), a1 = glm::radians(-15.0f + 30.0f * j);
                Renderer::drawLine({ sx + 8.0f + r * std::cos(a0), sy + r * std::sin(a0) },
                                   { sx + 8.0f + r * std::cos(a1), sy + r * std::sin(a1) }, 3.5f, UI_TEXT);
            }
        }
    }

    for (int i = 0; i < VOLUME_SEGMENTS; ++i) {
        float h = 14.0f + i * 3.5f;
        float x = left + i * (VOLUME_SEGMENT_W + VOLUME_SEGMENT_GAP) + VOLUME_SEGMENT_W * 0.5f;
        glm::vec4 color = i < filled ? (muted ? UI_TEXT_DIM : UI_ACCENT) : UI_PANEL_DARK;
        Renderer::drawRect({ x, VOLUME_METER_Y + h * 0.5f }, { VOLUME_SEGMENT_W + 4.0f, h + 4.0f }, UI_FRAME);
        Renderer::drawRect({ x, VOLUME_METER_Y + h * 0.5f }, { VOLUME_SEGMENT_W, h }, color);
    }

    float textX = left + meterW + 42.0f;
    if (muted) {
        Renderer::drawRect({ textX, VOLUME_METER_Y + 18.0f }, { 104.0f, 40.0f }, UI_FRAME);
        Renderer::drawRect({ textX, VOLUME_METER_Y + 18.0f }, { 98.0f, 34.0f }, UI_WARNING);
        Text::drawCentered("MUTED", textX, VOLUME_METER_Y + 6.0f, 0.65f, UI_TEXT);
    }
    else {
        int percent = static_cast<int>(volume * 100.0f + 0.5f);
        UiStyle::drawOutlinedText(std::to_string(percent) + "%", textX, VOLUME_METER_Y + 4.0f, 0.85f, UI_ACCENT, 2.0f);
    }
}

// how to play, next to the pause buttons (req 28, NF14)
static void drawControlsLegend(float panelTop, float panelBottom)
{
    glm::vec2 center(LEGEND_X, (panelTop + panelBottom) * 0.5f);
    UiStyle::drawPanel(center, { LEGEND_WIDTH, panelTop - panelBottom });
    UiStyle::drawOutlinedText("HOW TO PLAY", LEGEND_X, panelTop - 70.0f, 0.9f, PIZZA, 2.5f);

    const std::pair<const char*, const char*> rows[] = {
        { "HOVER LANE",  "HIGHLIGHTS THE WHOLE LANE" },
        { "CLICK LANE",  "SELECT / DESELECT (SEVERAL OK)" },
        { "CLICK CARD",  "DEPLOY ON THE LAST SELECTED LANE" },
        { "W / UP",      "SELECTED LANES JUMP" },
        { "S / DOWN",    "SELECTED LANES DUCK (HOLD)" },
        { "ESC",         "PAUSE / RESUME" },
    };
    float keyX = LEGEND_X - LEGEND_WIDTH * 0.5f + 30.0f;
    float descX = keyX + 150.0f;
    float descWidth = LEGEND_X + LEGEND_WIDTH * 0.5f - 25.0f - descX;
    float y = panelTop - 120.0f;
    for (const auto& row : rows) {
        Text::draw(row.first, keyX, y, 0.55f, UI_ACCENT);
        Text::draw(row.second, descX, y, UiStyle::fitScale(row.second, 0.5f, descWidth), UI_TEXT);
        y -= LEGEND_ROW_GAP;
    }

    Renderer::drawRect({ LEGEND_X, y + 18.0f }, { LEGEND_WIDTH - 60.0f, 3.0f }, UI_PANEL_LIGHT);
    y -= 22.0f;
    const char* tips[] = {
        "DUCK UNDER FOOT SHURIKEN (HIGH)",
        "JUMP OVER BEBOP'S BLASTS (LOW)",
        "WALK OVER PIZZA TO COLLECT IT",
        "DONATELLO SHIELDS TURTLES BEHIND HIM",
    };
    for (const char* tip : tips) {
        Text::drawCentered(tip, LEGEND_X, y, UiStyle::fitScale(tip, 0.48f, LEGEND_WIDTH - 60.0f), UI_TEXT_DIM);
        y -= LEGEND_ROW_GAP - 4.0f;
    }
}

// win: turtles hop for joy around the panel, the canister is safe
static void drawWinArt(float t)
{
    const float floorY = 110.0f;
    const TurtleKind kinds[] = { TurtleKind::Leonardo, TurtleKind::Raphael, TurtleKind::Donatello };
    const float xs[] = { 200.0f, 380.0f, 1260.0f };
    for (int i = 0; i < 3; ++i) {
        float hop = std::sin(t * 5.0f + i * 1.3f);
        float lift = hop > 0.0f ? hop * 60.0f : 0.0f;
        Pose p = figurePose(t, i * 0.7f, i != 2);
        p.inAir = lift > 1.0f;
        drawTurtleFigure(kinds[i], xs[i], floorY, MENU_TURTLE_SCALE, p, lift);
    }
    drawCanister(1440.0f, floorY, t);
}

// lose: the Foot Soldier walks off with the canister, Bebop + Rocksteady gloat
static void drawLoseArt(float t)
{
    const float floorY = 110.0f;
    drawCanister(200.0f, floorY, t);
    Pose walk = figurePose(t, 0.0f, false);
    walk.walkCycle = t * WALK_CYCLE_SPEED;
    drawVillainFigure(VillainKind::FootSoldier, 360.0f, floorY, MENU_VILLAIN_SCALE, walk);
    drawVillainFigure(VillainKind::Bebop,      1250.0f, floorY, MENU_VILLAIN_SCALE, figurePose(t, 0.4f, true));
    drawVillainFigure(VillainKind::Rocksteady, 1430.0f, floorY, MENU_VILLAIN_SCALE, figurePose(t, 1.9f, true));
}

// ================= Menu =================

MenuAction Menu::update(glm::vec2 mouseWorld, bool clicked)
{
    MenuAction result = MenuAction::None;
    bool mouseDown = Input::isMouseDown();
    for (Button& b : buttons) {
        b.hovered = b.contains(mouseWorld);
        b.pressed = b.hovered && mouseDown;
        if (b.hovered && clicked) result = b.action;
    }
    return result;
}

void Menu::draw() const
{
    float t = static_cast<float>(glfwGetTime());   // real time (Lab 2), so menu art keeps moving while the game is frozen

    if (overlay)
        Renderer::drawRect({ WORLD_WIDTH * 0.5f, WORLD_HEIGHT * 0.5f }, { WORLD_WIDTH, WORLD_HEIGHT }, withAlpha(BLACK, OVERLAY_DIM));

    if (kind == MenuKind::Main) {
        drawMainArt(t);
        if (!buttons.empty()) {
            float top = buttons.front().center.y + BUTTON_SIZE.y * 0.5f + 22.0f;
            float bottom = buttons.back().center.y - BUTTON_SIZE.y * 0.5f - 30.0f;
            UiStyle::drawPanel({ WORLD_WIDTH * 0.5f, (top + bottom) * 0.5f }, { BUTTON_SIZE.x + 80.0f, top - bottom });
        }
        for (const Button& b : buttons) b.draw();
        return;
    }

    if (kind == MenuKind::Win) drawWinArt(t);
    if (kind == MenuKind::Lose) drawLoseArt(t);

    // every other screen: one riveted panel holding title, extras and the button column
    float columnX = buttons.empty() ? WORLD_WIDTH * 0.5f : buttons.front().center.x;
    float top = TITLE_Y + PANEL_ABOVE_TITLE;
    float bottom = (buttons.empty() ? FIRST_BUTTON_Y : buttons.back().center.y) - BUTTON_SIZE.y * 0.5f - PANEL_BELOW_BUTTONS;
    UiStyle::drawPanel({ columnX, (top + bottom) * 0.5f }, { PANEL_WIDTH, top - bottom });

    if (kind == MenuKind::Pause) drawControlsLegend(top, bottom);

    UiStyle::drawOutlinedText(title, columnX, TITLE_Y, UiStyle::fitScale(title, TITLE_SCALE, PANEL_WIDTH - 50.0f), titleColor, 3.0f);
    if (!subtitle.empty())
        Text::drawCentered(subtitle, columnX, SUBTITLE_Y, UiStyle::fitScale(subtitle, 0.6f, PANEL_WIDTH - 50.0f), UI_TEXT);

    if (kind == MenuKind::Sound) drawVolumeMeter(columnX);

    for (const Button& b : buttons) b.draw();
}

// buttons stacked in a column under the title
static Menu makeMenu(MenuKind kind, const std::string& title, bool overlay, glm::vec4 titleColor,
                     float columnX, float firstY, float gap,
                     const std::vector<std::pair<std::string, MenuAction>>& items)
{
    Menu m;
    m.kind = kind;
    m.title = title;
    m.overlay = overlay;
    m.titleColor = titleColor;
    for (size_t i = 0; i < items.size(); ++i) {
        Button b;
        b.center = glm::vec2(columnX, firstY - i * gap);
        b.size = BUTTON_SIZE;
        b.label = items[i].first;
        b.action = items[i].second;
        m.buttons.push_back(b);
    }
    return m;
}

// themed: sewer background (drawn by Game), outlined title + ribbon, turtles vs villains around the mutagen (req 19)
Menu makeMainMenu()
{
    return makeMenu(MenuKind::Main, WINDOW_TITLE, false, MUTAGEN_GLOW, WORLD_WIDTH * 0.5f, MAIN_FIRST_BUTTON_Y, MAIN_BUTTON_GAP, {
        { "Start", MenuAction::Start },
        { "Sound Settings", MenuAction::SoundSettings },
        { "Exit", MenuAction::Exit },
    });
}

Menu makeSoundMenu()
{
    return makeMenu(MenuKind::Sound, "Sound Settings", false, PIZZA, WORLD_WIDTH * 0.5f, SOUND_FIRST_BUTTON_Y, BUTTON_GAP, {
        { "Volume +", MenuAction::VolumeUp },
        { "Volume -", MenuAction::VolumeDown },
        { "Mute / Unmute", MenuAction::ToggleMute },
        { "Back", MenuAction::Back },
    });
}

Menu makePauseMenu()
{
    Menu m = makeMenu(MenuKind::Pause, "Paused", true, PIZZA, PAUSE_COLUMN_X, FIRST_BUTTON_Y, BUTTON_GAP, {
        { "Continue", MenuAction::Continue },
        { "Restart", MenuAction::Restart },
        { "Main Menu", MenuAction::MainMenu },
    });
    m.subtitle = "The Foot Clan waits... Esc to resume";
    return m;
}

Menu makeWinMenu()
{
    Menu m = makeMenu(MenuKind::Win, "Lair Defended!", true, MUTAGEN_GLOW, WORLD_WIDTH * 0.5f, FIRST_BUTTON_Y, BUTTON_GAP, {
        { "Restart", MenuAction::Restart },
        { "Main Menu", MenuAction::MainMenu },
    });
    m.subtitle = "The mutagen is safe... for now!";
    return m;
}

Menu makeLoseMenu()
{
    Menu m = makeMenu(MenuKind::Lose, "The Lair Has Fallen", true, UI_WARNING, WORLD_WIDTH * 0.5f, FIRST_BUTTON_Y, BUTTON_GAP, {
        { "Restart", MenuAction::Restart },
        { "Main Menu", MenuAction::MainMenu },
    });
    m.subtitle = "Shredder has the mutagen. Try again!";
    return m;
}
