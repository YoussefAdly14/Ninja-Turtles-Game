#pragma once
#include "game/World.h"
#include "scene/Cutscene.h"
#include "ui/Hud.h"
#include "ui/Menu.h"
#include <string>

// One state machine owns update/render dispatch (req 9). Shape follows learnopengl Breakout's Game class.
enum class GameState { MainMenu, SoundSettings, Cutscene, Playing, Paused, Won, Lost };

class Game
{
public:
    GameState state = GameState::MainMenu;
    bool quitRequested = false;

    void init();       // after Renderer::init
    void shutdown();
    void update(float dt);
    void render();
    void startDemo();  // dev aid (--demo): straight into a game with turtles placed
    void devStart(const std::string& screen);   // dev aid: --cutscene / --pause / --win / --lose / --sound

private:
    World m_world;
    Cutscene m_cutscene;
    Hud m_hud;
    Menu m_mainMenu, m_soundMenu, m_pauseMenu, m_winMenu, m_loseMenu;
    float m_clock = 0.0f;      // real time, keeps menu backgrounds animated
    bool m_debugDraw = false;  // F1: hitboxes + FPS (req 16), off by default
    float m_fps = 60.0f;       // smoothed, for the debug overlay

    void startNewGame();
    void updatePlaying(float dt);
    void handleMenuAction(MenuAction action);
    void renderWorld();
    void renderDebug();
    void handleDebugKeys();
};
