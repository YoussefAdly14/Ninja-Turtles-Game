#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "ui/Button.h"

// Which screen a Menu is, so draw() can add that screen's artwork around the buttons.
enum class MenuKind { Main, Sound, Pause, Win, Lose };

// A title + a column of buttons (owner C, spec 4.3, req 18-28).
// update() returns the clicked button's action; Game::handleMenuAction decides what happens.
struct Menu
{
    std::string title;
    std::string subtitle;            // one line under the title (win/lose story beat)
    std::vector<Button> buttons;
    bool overlay = false;            // pause / win / lose: dim the frozen game behind it instead of a full background
    MenuKind kind = MenuKind::Main;
    glm::vec4 titleColor = glm::vec4(1.0f);

    MenuAction update(glm::vec2 mouseWorld, bool clicked);
    void draw() const;
};

Menu makeMainMenu();
Menu makeSoundMenu();
Menu makePauseMenu();
Menu makeWinMenu();
Menu makeLoseMenu();
