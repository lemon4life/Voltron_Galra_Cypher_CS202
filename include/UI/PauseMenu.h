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
    /// Creates a PauseMenu instance from the supplied configuration.
    PauseMenu();

    /// Advances this component's state for the current frame.
    PauseMenuAction Update(Vector2 mousePosition);
    /// Renders this component using its current state and visual resources.
    void Draw(Vector2 mousePosition) const;
};
