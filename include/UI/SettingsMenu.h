#pragma once

#include "UI/GUIButton.h"
#include "UI/GUISlider.h"

class SettingsMenu {
private:
    Rectangle containerBounds;
    GUISlider soundEffectsSlider;
    GUISlider musicSlider;
    GUIButton backButton;

public:
    SettingsMenu();

    bool Update(Vector2 mousePosition);
    void Draw(Vector2 mousePosition) const;
};
