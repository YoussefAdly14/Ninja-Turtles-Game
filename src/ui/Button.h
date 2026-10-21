#pragma once
#include <string>
#include <glm/glm.hpp>

// What a button asks the Game to do. Menus never change game state themselves.
enum class MenuAction
{
    None,
    Start, Exit, SoundSettings,            // main menu
    Continue, Restart, MainMenu,           // pause + win/lose menus
    VolumeUp, VolumeDown, ToggleMute, Back // sound settings (bonus)
};

// One button style reused by every menu (req 18, NF16).
struct Button
{
    glm::vec2 center;
    glm::vec2 size;
    std::string label;
    MenuAction action = MenuAction::None;
    bool hovered = false;

    bool contains(glm::vec2 p) const;
    void draw() const;
};
