#pragma once

#include "UI/GUIButton.h"

enum class PauseMenuAction {
    None,
    Resume,
    Settings,
    BackToMainMenu,
    Quit
};

class PauseMenu {
private:
    Rectangle containerBounds;
    GUIButton resumeButton;
    GUIButton settingsButton;
    GUIButton backToMainMenuButton;
    GUIButton quitButton;

public:
    PauseMenu();

    PauseMenuAction Update(Vector2 mousePosition);
    void Draw(Vector2 mousePosition) const;
};
