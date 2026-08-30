#pragma once

#include "UI/GUIButton.h"
#include "UI/GUISlider.h"

class SettingsMenu {
private:
    Rectangle containerBounds;
    GUISlider soundEffectsSlider;
    GUISlider musicSlider;
    Rectangle autoAimToggleBounds;
    GUIButton backButton;

public:
    /// Creates a SettingsMenu instance from the supplied configuration.
    SettingsMenu();

    /// Advances this component's state for the current frame.
    bool Update(Vector2 mousePosition);
    /// Renders this component using its current state and visual resources.
    void Draw(Vector2 mousePosition) const;
};
