#include "UI/GUIStatBar.h"

#include <algorithm>

void GUIStatBar::Draw(
    Rectangle bounds,
    const std::string& label,
    const std::string& exactValue,
    float value,
    float comparisonMaximum,
    Color fillColor,
    bool enabled
) {
    constexpr int FONT_SIZE = 12;
    DrawText(
        label.c_str(),
        static_cast<int>(bounds.x),
        static_cast<int>(bounds.y - FONT_SIZE - 2.0f),
        FONT_SIZE,
        enabled ? RAYWHITE : GRAY
    );

    int valueWidth = MeasureText(exactValue.c_str(), FONT_SIZE);
    DrawText(
        exactValue.c_str(),
        static_cast<int>(bounds.x + bounds.width - valueWidth),
        static_cast<int>(bounds.y - FONT_SIZE - 2.0f),
        FONT_SIZE,
        enabled ? Color{210, 220, 235, 255} : GRAY
    );

    DrawRectangleRec(bounds, Color{22, 28, 39, 255});
    DrawRectangleLinesEx(bounds, 1.0f, Color{120, 132, 151, 255});

    if (!enabled || comparisonMaximum <= 0.0f) {
        return;
    }

    float amount = std::clamp(value / comparisonMaximum, 0.0f, 1.0f);
    Rectangle fill = bounds;
    fill.width *= amount;
    DrawRectangleRec(fill, fillColor);
}
