#pragma once

#include "GUI/GUIButton.h"

enum class MainMenuAction {
    None,
    Play,
    Quit
};

class MainMenu {
private:
    GUIButton playButton;
    GUIButton quitButton;

public:
    MainMenu();

    MainMenuAction Update(Vector2 mousePosition);
    void Draw(Vector2 mousePosition) const;
};
