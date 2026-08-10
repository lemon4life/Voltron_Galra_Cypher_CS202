#include "UI/GUISlider.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"

#include <algorithm>
#include <cmath>
#include <utility>

GUISlider::GUISlider(Rectangle trackBounds, std::string label, float initialValue)
    : trackBounds(trackBounds),
      label(std::move(label)),
      value(0.0f),
      isDragging(false) {
    SetValue(initialValue);
}

bool GUISlider::Update(Vector2 mousePosition) {
    const Rectangle interactionBounds = {
        trackBounds.x - 8.0f,
        trackBounds.y - 10.0f,
        trackBounds.width + 16.0f,
        trackBounds.height + 20.0f
    };

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(mousePosition, interactionBounds)) {
        isDragging = true;
    }

    const float oldValue = value;
    if (isDragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        SetValue((mousePosition.x - trackBounds.x) / trackBounds.width);
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        isDragging = false;
    }

    return std::fabs(oldValue - value) > 0.0001f;
}

void GUISlider::Draw() const {
    constexpr float labelFontSize = 18.0f;
    const int percentage = static_cast<int>(value * 100.0f + 0.5f);
    const std::string percentageText = std::to_string(percentage) + "%";

    Font fontSans = AssetManager::GetInstance().GetCustomFont("PixeloidSans");
    Font fontMono = AssetManager::GetInstance().GetCustomFont("PixeloidMono");

    UIUtils::DrawText("PixeloidSans", label.c_str(), { trackBounds.x, trackBounds.y - 27.0f }, static_cast<UIUtils::FontSize>(labelFontSize), RAYWHITE);
    
    Vector2 pctSize = MeasureTextEx(fontMono, percentageText.c_str(), labelFontSize, 1.0f);
    UIUtils::DrawText("PixeloidMono", percentageText.c_str(), { trackBounds.x + trackBounds.width - pctSize.x, trackBounds.y - 27.0f }, static_cast<UIUtils::FontSize>(labelFontSize), LIGHTGRAY);

    DrawRectangleRec(trackBounds, Color{50, 54, 64, 255});
    DrawRectangleRec(
        Rectangle{trackBounds.x, trackBounds.y, trackBounds.width * value, trackBounds.height},
        GOLD);

    const Vector2 handleCenter = {
        trackBounds.x + trackBounds.width * value,
        trackBounds.y + trackBounds.height * 0.5f
    };
    DrawCircleV(handleCenter, 8.0f, RAYWHITE);
    DrawCircleLines(static_cast<int>(handleCenter.x),
                    static_cast<int>(handleCenter.y),
                    8.0f,
                    Color{90, 95, 108, 255});
}

void GUISlider::SetValue(float newValue) {
    value = std::clamp(newValue, 0.0f, 1.0f);
}

float GUISlider::GetValue() const {
    return value;
}
