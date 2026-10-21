#pragma once
#include <string>
#include <vector>
#include "ui/Button.h"

// A title + a column of buttons (owner C, spec 4.3, req 18-28).
// update() returns the clicked button's action; Game::handleMenuAction decides what happens.
struct Menu
{
    std::string title;
    std::vector<Button> buttons;
    bool overlay = false;   // pause / win / lose: dim the frozen game behind it instead of a full background

    MenuAction update(glm::vec2 mouseWorld, bool clicked);
    void draw() const;
};

Menu makeMainMenu();
Menu makeSoundMenu();
Menu makePauseMenu();
Menu makeWinMenu();
Menu makeLoseMenu();
