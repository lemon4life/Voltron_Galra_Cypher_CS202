#include "UI/GUIStatBar.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"

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
    constexpr float FONT_SIZE = 12.0f;
    Font fontSans = AssetManager::GetInstance().GetCustomFont("PixeloidSans");
    Font fontMono = AssetManager::GetInstance().GetCustomFont("PixeloidMono");

    UIUtils::DrawText("PixeloidSans", label.c_str(), { bounds.x, bounds.y - FONT_SIZE - 2.0f }, static_cast<UIUtils::FontSize>(FONT_SIZE), enabled ? RAYWHITE : GRAY);

    Vector2 valueSize = MeasureTextEx(fontMono, exactValue.c_str(), FONT_SIZE, 1.0f);
    UIUtils::DrawText("PixeloidMono", exactValue.c_str(), { bounds.x + bounds.width - valueSize.x, bounds.y - FONT_SIZE - 2.0f }, static_cast<UIUtils::FontSize>(FONT_SIZE), enabled ? Color{210, 220, 235, 255} : GRAY);

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
