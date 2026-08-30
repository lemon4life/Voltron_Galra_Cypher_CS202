#pragma once

#include "raylib.h"

#include <string>

class GUIButton {
private:
    Rectangle bounds;
    std::string label;

public:
    /// Creates a GUIButton instance from the supplied configuration.
    GUIButton(Rectangle bounds, std::string label);

    /// Advances this component's state for the current frame.
    bool Update(Vector2 mousePosition) const;
    /// Renders this component using its current state and visual resources.
    void Draw(Vector2 mousePosition) const;
};
