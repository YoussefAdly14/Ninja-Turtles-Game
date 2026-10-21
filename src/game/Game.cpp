#include "game/Game.h"
#include "core/Window.h"
#include "core/Input.h"
#include "core/Renderer.h"
#include "core/Random.h"
#include "core/Colors.h"
#include "models/TurtleModels.h"
#include "models/VillainModels.h"
#include "models/PropModels.h"
#include "models/LairModel.h"
#include "scene/Environment.h"
#include "ui/Text.h"
#include "ui/HealthBar.h"
#include "audio/Audio.h"
#include "Config.h"

void Game::init()
{
    seedRandom(RANDOM_SEED);

    // every module builds its meshes once here (NF4)
    TurtleModels::init();
    VillainModels::init();
    PropModels::init();
    LairModel::init();
    Environment::init();
    Text::init();
    Audio::init();

    m_mainMenu = makeMainMenu();
    m_soundMenu = makeSoundMenu();
    m_pauseMenu = makePauseMenu();
    m_winMenu = makeWinMenu();
    m_loseMenu = makeLoseMenu();

    startNewGame();
    state = GameState::MainMenu;
}

void Game::shutdown()
{
    Audio::shutdown();
    Text::shutdown();
    Environment::shutdown();
    LairModel::shutdown();
    PropModels::shutdown();
    VillainModels::shutdown();
    TurtleModels::shutdown();
}

// fresh coins, wave 1, no entities, no selection (req 20, 25)
void Game::startNewGame()
{
    m_world.reset();
    m_hud = Hud();
}

void Game::handleMenuAction(MenuAction action)
{
    switch (action) {
    case MenuAction::Start:
        startNewGame();
        m_cutscene.start();
        state = GameState::Cutscene;
        break;
    case MenuAction::Exit:          quitRequested = true;               break;
    case MenuAction::SoundSettings: state = GameState::SoundSettings;   break;
    case MenuAction::Back:          state = GameState::MainMenu;        break;
    case MenuAction::Continue:      state = GameState::Playing;         break;
    case MenuAction::Restart:       // OQ9: restart skips the cutscene
        startNewGame();
        state = GameState::Playing;
        break;
    case MenuAction::MainMenu:
        startNewGame();
        state = GameState::MainMenu;
        break;
    case MenuAction::VolumeUp:      Audio::volumeUp();   break;
    case MenuAction::VolumeDown:    Audio::volumeDown(); break;
    case MenuAction::ToggleMute:    Audio::toggleMute(); break;
    case MenuAction::None:          break;
    }
}

void Game::update(float dt)
{
    m_clock += dt;
    if (Input::wasKeyPressed(GLFW_KEY_F1)) m_debugDraw = !m_debugDraw;

    glm::vec2 mouse = Input::mouseWorld();
    bool clicked = Input::wasMouseClicked();

    // Only the current state's update runs, so lanes/cards never react behind a menu (req 54)
    // and Paused/Won/Lost freeze the whole world (req 23, 109).
    switch (state) {
    case GameState::MainMenu:
        handleMenuAction(m_mainMenu.update(mouse, clicked));
        break;
    case GameState::SoundSettings:
        handleMenuAction(m_soundMenu.update(mouse, clicked));
        break;
    case GameState::Cutscene:
        m_cutscene.update(dt);
        if (m_cutscene.finished()) state = GameState::Playing;
        break;
    case GameState::Playing:
        updatePlaying(dt);
        break;
    case GameState::Paused:
        if (Input::wasKeyPressed(GLFW_KEY_ESCAPE)) state = GameState::Playing;
        else handleMenuAction(m_pauseMenu.update(mouse, clicked));
        break;
    case GameState::Won:
        handleMenuAction(m_winMenu.update(mouse, clicked));
        break;
    case GameState::Lost:
        handleMenuAction(m_loseMenu.update(mouse, clicked));
        break;
    }
}

void Game::updatePlaying(float dt)
{
    if (Input::wasKeyPressed(GLFW_KEY_ESCAPE)) {
        state = GameState::Paused;
        return;
    }

    glm::vec2 mouse = Input::mouseWorld();
    bool clicked = Input::wasMouseClicked();

    // cards: buy + place on the most recently selected lane (req 66, OQ2)
    TurtleKind picked = m_hud.update(m_world, mouse, clicked);
    if (picked != TurtleKind::Count)
        m_world.placeTurtle(picked, m_world.lanes.lastSelectedLane);

    // lanes: hover + click to toggle (laneAt() is -1 over the HUD, so card clicks never toggle lanes)
    m_world.lanes.updateHover(mouse);
    if (clicked && m_world.lanes.hoveredLane >= 0)
        m_world.lanes.toggle(m_world.lanes.hoveredLane);

    bool jump = Input::wasKeyPressed(GLFW_KEY_W) || Input::wasKeyPressed(GLFW_KEY_UP);
    bool duck = Input::isKeyDown(GLFW_KEY_S) || Input::isKeyDown(GLFW_KEY_DOWN);
    applyJumpDuck(m_world.turtles, m_world.lanes, jump, duck);

    m_world.update(dt);

    // loss first: a villain at the Lair means the Lair fell, even if it was the last one (req 110)
    if (m_world.villainReachedLair())
        state = GameState::Lost;
    else if (m_world.allWavesCleared())
        state = GameState::Won;
}

void Game::render()
{
    Renderer::beginFrame(Window::projection());

    switch (state) {
    case GameState::MainMenu:
        Environment::drawBackground(m_clock);
        m_mainMenu.draw();
        break;
    case GameState::SoundSettings:
        Environment::drawBackground(m_clock);
        m_soundMenu.draw();
        break;
    case GameState::Cutscene:
        m_cutscene.draw();
        break;
    case GameState::Playing:
        renderWorld();
        break;
    case GameState::Paused:
        renderWorld();
        m_pauseMenu.draw();
        break;
    case GameState::Won:
        renderWorld();
        m_winMenu.draw();
        break;
    case GameState::Lost:
        renderWorld();
        m_loseMenu.draw();
        break;
    }
}

// Draw order (req 15): background -> environment -> lanes -> Lair -> pizza -> characters
// -> projectiles -> health bars -> HUD. Menus are drawn on top by render().
void Game::renderWorld()
{
    float t = m_world.time;

    Environment::drawBackground(t);
    Environment::drawProps(t);
    m_world.lanes.draw(t);
    LairModel::draw(t);

    for (const Collectible& c : m_world.collectibles) c.draw();

    // top lane first, so characters in lower lanes overlap the ones behind them
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        for (const Villain& v : m_world.villains) if (v.lane == lane) v.draw();
        for (const Turtle& tu : m_world.turtles) if (tu.lane == lane) tu.draw();
    }

    for (const Projectile& p : m_world.projectiles) p.draw();

    for (const Turtle& tu : m_world.turtles) drawHealthBar(tu.healthBarPosition(), tu.hp, tu.maxHp, true);
    for (const Villain& v : m_world.villains) drawHealthBar(v.healthBarPosition(), v.hp, v.maxHp, false);

    if (m_debugDraw) renderDebug();

    m_hud.draw(m_world);
}

// F1: hitboxes in translucent colours
void Game::renderDebug()
{
    auto box = [](const Entity& e, glm::vec4 color) {
        Renderer::drawRect({ e.x, e.bottom() + e.height * 0.5f }, { e.width, e.height }, color);
    };
    for (const Turtle& t : m_world.turtles) box(t, glm::vec4(0.0f, 1.0f, 0.0f, 0.3f));
    for (const Villain& v : m_world.villains) box(v, glm::vec4(1.0f, 0.0f, 0.0f, 0.3f));
    for (const Projectile& p : m_world.projectiles) box(p, glm::vec4(1.0f, 1.0f, 0.0f, 0.4f));
    for (const Collectible& c : m_world.collectibles) box(c, glm::vec4(0.0f, 0.6f, 1.0f, 0.4f));
}
