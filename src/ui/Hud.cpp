#include "ui/Hud.h"
#include "ui/Text.h"
#include "ui/UiStyle.h"
#include "game/World.h"
#include "core/Input.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "models/TurtleModels.h"
#include "models/PropModels.h"
#include "Config.h"
#include <cmath>
#include <string>
#include <utility>

// ---- layout (HUD = the top HUD_HEIGHT units, centre line HUD_Y) ----
const float HUD_Y = WORLD_HEIGHT - HUD_HEIGHT * 0.5f;
const glm::vec2 CARD_SIZE(118.0f, 122.0f);
const float CARD_GAP = 12.0f;
const float CARDS_LEFT = 18.0f;
const float CARD_HOVER_LIFT = 3.0f;
const float PORTRAIT_SCALE = 0.5f;
const float HINT_X = 548.0f;                   // hint / tooltip plate between the cards and the wave plate
const float HINT_MAX_WIDTH = 270.0f;
const glm::vec2 WAVE_PLATE_SIZE(200.0f, 104.0f);
const float CONTROLS_X = 1135.0f;              // small controls legend (req 28)
const glm::vec2 PIZZA_PLATE_CENTER(1480.0f, HUD_Y);
const glm::vec2 PIZZA_PLATE_SIZE(210.0f, 92.0f);
const float PIPE_HEIGHT = 14.0f;               // pipe along the bottom edge of the HUD

// ---- timings (seconds of world.time) ----
const float HINT_DURATION = 2.0f;
const float HINT_POP = 0.15f;
const float CARD_FLASH_DURATION = 0.3f;
const float COIN_PULSE_DURATION = 0.45f;
const float COIN_PULSE_GROW = 0.3f;
const float COIN_FLOAT_DURATION = 0.9f;
const float BANNER_DURATION = FIRST_SPAWN_DELAY < WAVE_BREAK_DURATION ? FIRST_SPAWN_DELAY : WAVE_BREAK_DURATION;
const float BANNER_SLIDE = 0.35f;
const float BANNER_Y = (PLAYFIELD_BOTTOM + PLAYFIELD_TOP) * 0.5f;
const float BANNER_HEIGHT = 150.0f;

// ---- card data ----
const glm::vec4 CARD_BODY = hexColor(0x4A3C2E);
const char* const CARD_NAMES[] = { "LEONARDO", "RAPHAEL", "DONATELLO" };
const char* const CARD_ROLES[] = { "MELEE", "RANGED", "DEFENSE" };
const char* const CARD_INFO[] = { "TWIN KATANAS, CLOSE SLASH", "THROWS SPINNING SAI", "STANDS STILL, BLOCKS 7 HITS" };
const char* const WAVE_TAGLINES[] = {
    "THE FOOT CLAN IS IN THE TUNNELS!",
    "BEBOP JOINS IN - JUMP HIS LOW BLASTS!",
    "FINAL WAVE - ROCKSTEADY IS CHARGING!",
};

static glm::vec4 maskColor(TurtleKind kind)
{
    if (kind == TurtleKind::Raphael) return RAPHAEL_MASK;
    if (kind == TurtleKind::Donatello) return DONATELLO_MASK;
    return LEONARDO_MASK;
}

static glm::vec2 cardCenter(int i)
{
    return glm::vec2(CARDS_LEFT + CARD_SIZE.x * 0.5f + i * (CARD_SIZE.x + CARD_GAP), HUD_Y);
}

static bool insideCard(int i, glm::vec2 p)
{
    glm::vec2 c = cardCenter(i);
    return p.x >= c.x - CARD_SIZE.x * 0.5f && p.x <= c.x + CARD_SIZE.x * 0.5f &&
           p.y >= c.y - CARD_SIZE.y * 0.5f && p.y <= c.y + CARD_SIZE.y * 0.5f;
}

static float clamp01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

static float smooth(float v)   // ease in/out (smoothstep curve)
{
    v = clamp01(v);
    return v * v * (3.0f - 2.0f * v);
}

void Hud::showHint(const World& world, const std::string& text, const std::string& detail)
{
    m_hint = text;
    m_hintDetail = detail;
    m_hintTime = world.time;
}

TurtleKind Hud::update(const World& world, glm::vec2 mouseWorld, bool clicked)
{
    // pizza changed since last frame -> pulse + floating "+15"/"-10" (req 60)
    if (m_lastCoins >= 0 && world.economy.coins != m_lastCoins) {
        m_coinDelta = world.economy.coins - m_lastCoins;
        m_coinTime = world.time;
    }
    m_lastCoins = world.economy.coins;

    // a new wave is about to start -> banner (req 103)
    if (world.waves.isBetweenWaves() && world.waves.waveNumber() != m_bannerWave) {
        m_bannerWave = world.waves.waveNumber();
        m_bannerTime = world.time;
    }

    hoveredCard = -1;
    int lockedClicked = -1;
    for (int i = 0; i < (int)TurtleKind::Count; ++i) {
        if (!insideCard(i, mouseWorld)) continue;
        // locked cards ignore hover and clicks (req 62); a click only explains why (NF15)
        if (world.economy.isUnlocked((TurtleKind)i)) hoveredCard = i;
        else if (clicked) lockedClicked = i;
    }
    m_cardPressed = hoveredCard >= 0 && Input::isMouseDown();

    if (lockedClicked >= 0) {
        TurtleKind kind = (TurtleKind)lockedClicked;
        showHint(world, "NEED " + std::to_string(turtleCost(kind)) + " PIZZA!",
                 std::string(CARD_NAMES[lockedClicked]) + " UNLOCKS AT " + std::to_string(turtleCost(kind)));
    }
    if (!clicked || hoveredCard < 0) return TurtleKind::Count;

    // no lane selected: explain and spend nothing (req 66, NF15)
    if (world.lanes.lastSelectedLane < 0) {
        showHint(world, "SELECT A LANE FIRST!", "CLICK A LANE, THEN THE CARD");
        return TurtleKind::Count;
    }
    m_hintTime = -100.0f;   // a successful deploy clears an old hint
    m_flashCard = hoveredCard;
    m_flashTime = world.time;
    return (TurtleKind)hoveredCard;
}

// ================= drawing =================

void Hud::drawCard(int i, const World& world) const
{
    TurtleKind kind = (TurtleKind)i;
    bool unlocked = world.economy.isUnlocked(kind);
    bool hovered = i == hoveredCard;
    glm::vec2 c = cardCenter(i);
    if (hovered) c.y += m_cardPressed ? -CARD_HOVER_LIFT : CARD_HOVER_LIFT;

    // locked = grayscale through the shader, not a grey box on top (req 62); text can't be greyed
    // by the shader, so locked text just uses the grey UI_TEXT_DIM
    Renderer::setGrayscale(unlocked ? 0.0f : 1.0f);
    glm::vec4 text = unlocked ? UI_TEXT : UI_TEXT_DIM;

    Renderer::drawRoundedRect(c, CARD_SIZE + glm::vec2(hovered ? 10.0f : 6.0f), 12.0f, hovered ? UI_ACCENT : UI_FRAME);
    Renderer::drawRoundedRect(c, CARD_SIZE, 9.0f, m_cardPressed && hovered ? hexColor(0x3A2F24) : CARD_BODY);

    // name band in the turtle's mask colour (req 61)
    glm::vec2 band(c.x, c.y + CARD_SIZE.y * 0.5f - 15.0f);
    Renderer::drawRect(band, { CARD_SIZE.x - 10.0f, 24.0f }, maskColor(kind));
    Renderer::drawRect(band - glm::vec2(0.0f, 12.0f), { CARD_SIZE.x - 10.0f, 2.0f }, UI_FRAME);
    float nameScale = UiStyle::fitScale(CARD_NAMES[i], 0.45f, CARD_SIZE.x - 18.0f);
    Text::drawCentered(CARD_NAMES[i], c.x, band.y - 8.0f, nameScale, text);

    // portrait = the real model, scaled down, in a dark sewer window
    Renderer::drawRect({ c.x, c.y + 3.0f }, { CARD_SIZE.x - 14.0f, 62.0f }, UI_PANEL_DARK);
    glm::mat4 base = translate2D(glm::mat4(1.0f), c.x - 6.0f, c.y - 27.0f);
    Pose idle;
    idle.time = world.time;
    TurtleModels::draw(kind, scale2D(base, PORTRAIT_SCALE, PORTRAIT_SCALE), idle);

    // cost row: pizza icon + price
    glm::mat4 icon = translate2D(glm::mat4(1.0f), c.x - 24.0f, c.y - 44.0f);
    PropModels::drawPizza(scale2D(icon, 0.55f, 0.55f), 0.0f);
    Text::draw(std::to_string(turtleCost(kind)), c.x - 8.0f, c.y - 54.0f, 0.5f, unlocked ? PIZZA : UI_TEXT_DIM);

    if (!unlocked) UiStyle::drawPadlock({ c.x + 36.0f, c.y + 16.0f }, 30.0f);
    Renderer::setGrayscale(0.0f);

    // white flash on the card that just deployed a turtle (NF12)
    float flash = 1.0f - (world.time - m_flashTime) / CARD_FLASH_DURATION;
    if (i == m_flashCard && flash > 0.0f && flash <= 1.0f)
        Renderer::drawRect(c, CARD_SIZE, withAlpha(WHITE, 0.6f * flash));
}

// invalid-click hint (red) or the hovered card's role (dark), between the cards and the wave plate
void Hud::drawHintArea(const World& world) const
{
    float age = world.time - m_hintTime;
    bool hint = age >= 0.0f && age < HINT_DURATION;
    if (!hint && hoveredCard < 0) return;

    std::string line1 = hint ? m_hint : std::string(CARD_NAMES[hoveredCard]) + " - " + CARD_ROLES[hoveredCard];
    std::string line2 = hint ? m_hintDetail : CARD_INFO[hoveredCard];
    float s1 = UiStyle::fitScale(line1, 0.6f, HINT_MAX_WIDTH - 24.0f);
    float s2 = UiStyle::fitScale(line2, 0.4f, HINT_MAX_WIDTH - 24.0f);
    float pop = hint ? 1.0f + 0.15f * (1.0f - clamp01(age / HINT_POP)) : 1.0f;   // pops in, then settles

    glm::vec2 size(HINT_MAX_WIDTH * pop, 74.0f * pop);
    glm::vec2 c(HINT_X, HUD_Y);
    Renderer::drawRoundedRect(c, size + glm::vec2(8.0f), 12.0f, UI_FRAME);
    Renderer::drawRoundedRect(c, size, 9.0f, hint ? UI_WARNING : UI_PANEL);
    if (!hint) Renderer::drawRect({ c.x - size.x * 0.5f + 6.0f, c.y }, { 6.0f, size.y - 16.0f }, maskColor((TurtleKind)hoveredCard));

    UiStyle::drawOutlinedText(line1, c.x, c.y + 4.0f, s1 * pop, UI_TEXT, 2.0f);
    Text::drawCentered(line2, c.x, c.y - 26.0f, s2 * pop, hint ? UI_TEXT : UI_TEXT_DIM);
}

void Hud::drawPizzaCounter(const World& world) const
{
    UiStyle::drawPanel(PIZZA_PLATE_CENTER, PIZZA_PLATE_SIZE);

    float age = world.time - m_coinTime;
    float pulse = (age >= 0.0f && age < COIN_PULSE_DURATION) ? std::sin(3.14159265f * age / COIN_PULSE_DURATION) : 0.0f;
    float grow = 1.0f + COIN_PULSE_GROW * pulse;
    glm::vec4 flashColor = m_coinDelta > 0 ? UI_ACCENT : UI_WARNING;
    glm::vec4 numberColor = glm::mix(PIZZA, flashColor, pulse);

    glm::mat4 icon = translate2D(glm::mat4(1.0f), PIZZA_PLATE_CENTER.x - 62.0f, HUD_Y + 2.0f);
    PropModels::drawPizza(scale2D(icon, 1.3f * grow, 1.3f * grow), 0.0f);

    std::string number = std::to_string(world.economy.coins);
    float scale = UiStyle::fitScale(number, 1.0f, 120.0f) * grow;
    float x = PIZZA_PLATE_CENTER.x - 30.0f;
    // outline by hand (drawOutlinedText is centred, the number is left-aligned so it doesn't jump around)
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx != 0 || dy != 0) Text::draw(number, x + dx * 2.5f, HUD_Y - 17.0f + dy * 2.5f, scale, BLACK);
    Text::draw(number, x, HUD_Y - 17.0f, scale, numberColor);

    // "+15" / "-10" floats up and fades
    float floatAge = world.time - m_coinTime;
    if (m_coinDelta != 0 && floatAge >= 0.0f && floatAge < COIN_FLOAT_DURATION) {
        float k = floatAge / COIN_FLOAT_DURATION;
        std::string delta = (m_coinDelta > 0 ? "+" : "") + std::to_string(m_coinDelta);
        Text::draw(delta, x + Text::width(number, scale) + 8.0f, HUD_Y + 4.0f + 28.0f * k, 0.6f, withAlpha(flashColor, 1.0f - k));
    }
}

// "WAVE 2/3" slides in from the right over a dark band, holds, slides out left (req 103)
void Hud::drawWaveBanner(const World& world) const
{
    float age = world.time - m_bannerTime;
    if (m_bannerWave <= 0 || age < 0.0f || age > BANNER_DURATION) return;

    float in = smooth(age / BANNER_SLIDE);
    float out = smooth((age - (BANNER_DURATION - BANNER_SLIDE)) / BANNER_SLIDE);
    float shown = in < 1.0f - out ? in : 1.0f - out;
    float x = WORLD_WIDTH * 0.5f + (1.0f - in) * WORLD_WIDTH * 0.6f - out * WORLD_WIDTH * 0.6f;

    float h = BANNER_HEIGHT * shown;
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, BANNER_Y }, { WORLD_WIDTH, h }, withAlpha(BLACK, 0.6f));
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, BANNER_Y + h * 0.5f }, { WORLD_WIDTH, 5.0f }, withAlpha(MUTAGEN_GLOW, shown));
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, BANNER_Y - h * 0.5f }, { WORLD_WIDTH, 5.0f }, withAlpha(MUTAGEN_GLOW, shown));

    std::string text = "WAVE " + std::to_string(m_bannerWave) + "/" + std::to_string(WAVE_COUNT);
    UiStyle::drawOutlinedText(text, x, BANNER_Y - 10.0f, 2.2f, MUTAGEN_GLOW, 4.0f);
    int tagline = m_bannerWave - 1;
    if (tagline < (int)(sizeof(WAVE_TAGLINES) / sizeof(WAVE_TAGLINES[0])))
        UiStyle::drawOutlinedText(WAVE_TAGLINES[tagline], x, BANNER_Y - 55.0f, 0.65f, UI_TEXT, 2.0f);
}

// top bar: cards top-left, wave top-centre, pizza top-right; never below PLAYFIELD_TOP (req 119)
void Hud::draw(const World& world) const
{
    drawWaveBanner(world);   // over the playfield, under the bar

    Renderer::drawRect({ WORLD_WIDTH * 0.5f, HUD_Y }, { WORLD_WIDTH, HUD_HEIGHT }, UI_HUD_BAR);
    // a sewer pipe runs along the bottom edge, with flanges
    float pipeY = PLAYFIELD_TOP + PIPE_HEIGHT * 0.5f;
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, pipeY }, { WORLD_WIDTH, PIPE_HEIGHT }, UI_PIPE);
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, pipeY + 3.0f }, { WORLD_WIDTH, 3.0f }, withAlpha(WHITE, 0.15f));
    Renderer::drawRect({ WORLD_WIDTH * 0.5f, PLAYFIELD_TOP + 1.0f }, { WORLD_WIDTH, 2.0f }, UI_FRAME);
    for (float fx = 100.0f; fx < WORLD_WIDTH; fx += 200.0f) {
        Renderer::drawRect({ fx, pipeY }, { 16.0f, PIPE_HEIGHT + 8.0f }, UI_FRAME);
        Renderer::drawRect({ fx, pipeY }, { 12.0f, PIPE_HEIGHT + 4.0f }, hexColor(0x6E7D76));
    }

    for (int i = 0; i < (int)TurtleKind::Count; ++i) drawCard(i, world);
    drawHintArea(world);

    // wave indicator: "WAVE 2/3" + one pip per wave (done = green, current = pulsing pizza)
    glm::vec2 wave(WORLD_WIDTH * 0.5f, HUD_Y + 4.0f);
    UiStyle::drawPanel(wave, WAVE_PLATE_SIZE);
    UiStyle::drawOutlinedText("WAVE " + std::to_string(world.waves.waveNumber()) + "/" + std::to_string(WAVE_COUNT),
                              wave.x, wave.y + 2.0f, 0.9f, UI_TEXT, 2.0f);
    for (int w = 0; w < WAVE_COUNT; ++w) {
        glm::vec2 pip(wave.x + (w - (WAVE_COUNT - 1) * 0.5f) * 30.0f, wave.y - 28.0f);
        int current = world.waves.waveNumber() - 1;
        glm::vec4 color = UI_PANEL_DARK;
        if (w < current || world.waves.finished()) color = UI_ACCENT;
        else if (w == current) color = glm::mix(PIZZA, WHITE, 0.3f + 0.3f * std::sin(world.time * 6.0f));
        Renderer::drawCircle(pip, 10.0f, UI_FRAME);
        Renderer::drawCircle(pip, 7.0f, color);
    }

    // controls hint (req 28): keys in green, words in grey
    Text::drawCentered("CLICK A LANE, THEN A CARD TO DEPLOY", CONTROLS_X, HUD_Y + 10.0f, 0.45f, UI_TEXT_DIM);
    const std::pair<const char*, bool> parts[] = {
        { "W", true }, { " JUMP     ", false }, { "S", true }, { " DUCK     ", false }, { "ESC", true }, { " PAUSE", false },
    };
    float total = 0.0f;
    for (const auto& p : parts) total += Text::width(p.first, 0.45f);
    float px = CONTROLS_X - total * 0.5f;
    for (const auto& p : parts) {
        Text::draw(p.first, px, HUD_Y - 28.0f, 0.45f, p.second ? UI_ACCENT : UI_TEXT_DIM);
        px += Text::width(p.first, 0.45f);
    }

    drawPizzaCounter(world);
}
