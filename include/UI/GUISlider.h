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
    GUISlider(Rectangle trackBounds, std::string label, float initialValue);

    bool Update(Vector2 mousePosition);
    void Draw() const;

    void SetValue(float newValue);
    float GetValue() const;
};
