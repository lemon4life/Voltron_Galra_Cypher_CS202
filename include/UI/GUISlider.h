#pragma once

#include "raylib.h"

#include <string>

class GUISlider {
private:
    Rectangle trackBounds;
    std::string label;
    float value;
    bool isDragging;

public:
    /// Creates a GUISlider instance from the supplied configuration.
    GUISlider(Rectangle trackBounds, std::string label, float initialValue);

    /// Advances this component's state for the current frame.
    bool Update(Vector2 mousePosition);
    /// Renders this component using its current state and visual resources.
    void Draw() const;

    /// Updates the stored value.
    void SetValue(float newValue);
    /// Returns the current value.
    float GetValue() const;
};
