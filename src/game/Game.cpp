#include "game/Game.h"
#include "core/Window.h"
#include "core/Input.h"
#include "core/Renderer.h"
#include "core/Random.h"
#include "core/Colors.h"
#include "core/Transform.h"
#include "models/TurtleModels.h"
#include "models/VillainModels.h"
#include "models/PropModels.h"
#include "models/LairModel.h"
#include "scene/Environment.h"
#include "ui/Text.h"
#include "ui/HealthBar.h"
#include "audio/Audio.h"
#include "Config.h"

const float GHOST_BLOCKED_TINT = 0.7f;   // how red the placement preview gets over a refused spot

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
    m_placingDonatello = false;
}

// Dev aid for testing combat without clicking: fixed seed, lots of pizza, a turtle setup that
// exercises melee, ranged, Donatello protecting a Leonardo behind him, and every lane.
void Game::startDemo()
{
    seedRandom(1);
    startNewGame();
    m_world.economy.add(1000);
    m_world.placeTurtle(TurtleKind::Leonardo, 0);
    m_world.placeTurtle(TurtleKind::Raphael, 1);
    m_world.placeTurtle(TurtleKind::Leonardo, 2);
    m_world.placeTurtle(TurtleKind::Donatello, 2);
    m_world.placeTurtle(TurtleKind::Raphael, 3);
    state = GameState::Playing;
}

// Dev aid: open one screen directly so it can be screenshotted without clicking (tools/capture.ps1).
// Pause/win/lose sit over a demo game that has already run for a while, so the scene isn't empty.
void Game::devStart(const std::string& screen)
{
    const float DEV_WARMUP_SECONDS = 12.0f;
    const float DEV_WARMUP_STEP = 1.0f / 60.0f;

    if (screen == "cutscene") {
        startNewGame();
        m_cutscene.start();
        state = GameState::Cutscene;
        return;
    }
    if (screen == "sound") {
        state = GameState::SoundSettings;
        return;
    }

    startDemo();
    for (float t = 0.0f; t < DEV_WARMUP_SECONDS; t += DEV_WARMUP_STEP) m_world.update(DEV_WARMUP_STEP);
    if (screen == "pause") state = GameState::Paused;
    else if (screen == "win") state = GameState::Won;
    else if (screen == "lose") state = GameState::Lost;
}

// F2-F5 testing keys (req 17), only while playing. Turn off with DEV_KEYS_ENABLED before submission.
//   F2 +pizza   F3 defeat every villain on screen (they still drop pizza)   F4 lose now   F5 end this wave's spawning
void Game::handleDebugKeys()
{
    if (!DEV_KEYS_ENABLED) return;
    if (Input::wasKeyPressed(GLFW_KEY_F2)) m_world.economy.add(DEV_COIN_BONUS);
    if (Input::wasKeyPressed(GLFW_KEY_F3))
        for (Villain& v : m_world.villains) v.takeDamage(v.hp);
    if (Input::wasKeyPressed(GLFW_KEY_F4)) state = GameState::Lost;
    if (Input::wasKeyPressed(GLFW_KEY_F5)) m_world.waves.skipRestOfWave();
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
    if (dt > 0.0f) m_fps = m_fps * 0.95f + (1.0f / dt) * 0.05f;
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
        // optional skip (req 37, OQ10: switch off in Config.h if the TA says no)
        if (CUTSCENE_SKIP_ENABLED && (Input::wasKeyPressed(GLFW_KEY_SPACE) || Input::wasKeyPressed(GLFW_KEY_ENTER)))
            m_cutscene.skip();
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
        m_placingDonatello = false;
        m_hud.activeCard = -1;
        return;
    }
    handleDebugKeys();
    if (state != GameState::Playing) return;

    glm::vec2 mouse = Input::mouseWorld();
    bool clicked = Input::wasMouseClicked();

    // cards: buy + place on the most recently selected lane (req 66, OQ2). Donatello's card instead
    // switches placement mode on/off: he is bought when a spot in a lane is clicked.
    TurtleKind picked = m_hud.update(m_world, mouse, clicked);
    if (DONATELLO_FREE_PLACEMENT && picked == TurtleKind::Donatello) {
        m_placingDonatello = !m_placingDonatello;
    }
    else if (picked != TurtleKind::Count) {
        m_placingDonatello = false;   // another card was clicked
        m_world.placeTurtle(picked, m_world.lanes.lastSelectedLane);
    }
    if (!m_world.economy.isUnlocked(TurtleKind::Donatello)) m_placingDonatello = false;   // card relocked (req 63)

    // lanes: hover + click to toggle (laneAt() is -1 over the HUD, so card clicks never toggle lanes).
    // In placement mode the click puts Donatello at the cursor in the lane under it instead.
    m_world.lanes.updateHover(mouse);
    if (m_placingDonatello && Input::wasRightMouseClicked()) m_placingDonatello = false;   // cancel
    if (clicked && m_world.lanes.hoveredLane >= 0 && picked == TurtleKind::Count) {
        if (m_placingDonatello) {
            int lane = m_world.lanes.hoveredLane;
            PlaceResult result = m_world.checkPlacement(TurtleKind::Donatello, lane, mouse.x);
            if (result == PlaceResult::Ok) {
                m_world.placeTurtle(TurtleKind::Donatello, lane, mouse.x);
                m_placingDonatello = false;
            }
            else if (result == PlaceResult::BlockedByDonatello) {   // stay in placement mode: pick another spot
                m_hud.showHint(m_world, "TOO CLOSE!", "ANOTHER DONATELLO IS THERE");
            }
            else if (result == PlaceResult::BlockedByVillain) {
                m_hud.showHint(m_world, "ENEMY IN THE WAY!", "PICK A SPOT AWAY FROM THEM");
            }
        }
        else {
            m_world.lanes.toggle(m_world.lanes.hoveredLane);
        }
    }
    m_hud.activeCard = m_placingDonatello ? (int)TurtleKind::Donatello : -1;

    // preview of the placement click: same check the click makes, so the colour never disagrees with it
    m_ghostLane = -1;
    if (m_placingDonatello && m_world.lanes.hoveredLane >= 0) {
        m_ghostLane = m_world.lanes.hoveredLane;
        m_ghostX = glm::clamp(mouse.x, DONATELLO_MIN_X, DONATELLO_MAX_X);
        m_ghostValid = m_world.checkPlacement(TurtleKind::Donatello, m_ghostLane, mouse.x) == PlaceResult::Ok;
    }

    bool jump = Input::wasKeyPressed(GLFW_KEY_W) || Input::wasKeyPressed(GLFW_KEY_UP);
    bool duck = Input::isKeyDown(GLFW_KEY_S) || Input::isKeyDown(GLFW_KEY_DOWN);
    applyJumpDuck(m_world.turtles, m_world.lanes, jump, duck);

    m_world.update(dt);

    // loss first: a villain at the Lair means the Lair fell, even if it was the last one (req 110)
    if (m_world.villainReachedLair())
        state = GameState::Lost;
    else if (m_world.allWavesCleared())
        state = GameState::Won;

    if (state != GameState::Playing) {   // game over: no placement mode behind the Win/Lose screen
        m_placingDonatello = false;
        m_hud.activeCard = -1;
    }
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

    // Donatello placement preview: his model at the cursor + a strip on the floor, green = a click works,
    // red (damage tint) = it would be refused. On top of the characters, like a cursor.
    if (m_placingDonatello && m_ghostLane >= 0) {
        glm::vec4 strip = m_ghostValid ? withAlpha(MUTAGEN_GLOW, 0.6f) : hexColor(0xE03A3A, 0.6f);
        Renderer::drawRect({ m_ghostX, laneFloorY(m_ghostLane) }, { TURTLE_WIDTH, 10.0f }, strip);
        Pose idle;
        idle.time = t;
        Renderer::setDamageTint(m_ghostValid ? 0.0f : GHOST_BLOCKED_TINT);
        TurtleModels::draw(TurtleKind::Donatello, translate2D(glm::mat4(1.0f), m_ghostX, laneFloorY(m_ghostLane)), idle);
        Renderer::setDamageTint(0.0f);
    }

    for (const Projectile& p : m_world.projectiles) p.draw();
    m_world.effects.draw();   // smoke + sparks (req 68, 83, 120): over characters, under health bars

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

    // FPS in the water channel, where the HUD can't cover it (NF3 check). dt is clamped, so it reads >= 20.
    Text::draw("FPS " + std::to_string(static_cast<int>(m_fps + 0.5f)), 12.0f, 18.0f, 0.5f, WHITE);
}
