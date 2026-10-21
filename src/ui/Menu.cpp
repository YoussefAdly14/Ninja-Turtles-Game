#include "ui/Menu.h"
#include "ui/Text.h"
#include "core/Renderer.h"
#include "core/Colors.h"
#include "Config.h"
#include <utility>

const glm::vec2 BUTTON_SIZE(340.0f, 70.0f);
const float BUTTON_GAP = 95.0f;
const float FIRST_BUTTON_Y = 470.0f;
const float TITLE_Y = 640.0f;

MenuAction Menu::update(glm::vec2 mouseWorld, bool clicked)
{
    MenuAction result = MenuAction::None;
    for (Button& b : buttons) {
        b.hovered = b.contains(mouseWorld);
        if (b.hovered && clicked) result = b.action;
    }
    return result;
}

void Menu::draw() const
{
    if (overlay)
        Renderer::drawRect({ WORLD_WIDTH * 0.5f, WORLD_HEIGHT * 0.5f }, { WORLD_WIDTH, WORLD_HEIGHT }, withAlpha(BLACK, 0.6f));

    Text::drawCentered(title, WORLD_WIDTH * 0.5f, TITLE_Y, 1.5f, WHITE);
    for (const Button& b : buttons) b.draw();
}

// buttons stacked in a column under the title
static Menu makeMenu(const std::string& title, bool overlay,
                     const std::vector<std::pair<std::string, MenuAction>>& items)
{
    Menu m;
    m.title = title;
    m.overlay = overlay;
    for (size_t i = 0; i < items.size(); ++i) {
        Button b;
        b.center = glm::vec2(WORLD_WIDTH * 0.5f, FIRST_BUTTON_Y - i * BUTTON_GAP);
        b.size = BUTTON_SIZE;
        b.label = items[i].first;
        b.action = items[i].second;
        m.buttons.push_back(b);
    }
    return m;
}

// TODO(C): themed main menu: sewer background, title art, turtle silhouettes (req 19)
Menu makeMainMenu()
{
    return makeMenu(WINDOW_TITLE, false, {
        { "Start", MenuAction::Start },
        { "Sound Settings", MenuAction::SoundSettings },
        { "Exit", MenuAction::Exit },
    });
}

// TODO(C): show the current volume as a bar or % (req 127)
Menu makeSoundMenu()
{
    return makeMenu("Sound Settings", false, {
        { "Volume +", MenuAction::VolumeUp },
        { "Volume -", MenuAction::VolumeDown },
        { "Mute / Unmute", MenuAction::ToggleMute },
        { "Back", MenuAction::Back },
    });
}

Menu makePauseMenu()
{
    return makeMenu("Paused", true, {
        { "Continue", MenuAction::Continue },
        { "Restart", MenuAction::Restart },
        { "Main Menu", MenuAction::MainMenu },
    });
}

Menu makeWinMenu()
{
    return makeMenu("Lair Defended!", true, {
        { "Restart", MenuAction::Restart },
        { "Main Menu", MenuAction::MainMenu },
    });
}

Menu makeLoseMenu()
{
    return makeMenu("The Lair Has Fallen", true, {
        { "Restart", MenuAction::Restart },
        { "Main Menu", MenuAction::MainMenu },
    });
}
