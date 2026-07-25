#pragma once

#include "UI/GUIButton.h"
#include "UI/GUISlider.h"

enum class PauseMenuAction {
    None,
    Resume,
    BackToMainMenu,
    Quit
};

class PauseMenu {
private:
    Rectangle containerBounds;
    GUISlider soundEffectsSlider;
    GUISlider musicSlider;
    GUIButton resumeButton;
    GUIButton backToMainMenuButton;
    GUIButton quitButton;

public:
    PauseMenu();

    PauseMenuAction Update(Vector2 mousePosition);
    void Draw(Vector2 mousePosition) const;
};
