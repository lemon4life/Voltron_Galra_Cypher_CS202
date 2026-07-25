#pragma once

#include "raylib.h"

#include <string>

class GUIButton {
private:
    Rectangle bounds;
    std::string label;

public:
    GUIButton(Rectangle bounds, std::string label);

    bool Update(Vector2 mousePosition) const;
    void Draw(Vector2 mousePosition) const;
};
