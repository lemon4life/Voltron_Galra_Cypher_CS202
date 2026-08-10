#include "UI/GUIButton.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"

#include <utility>

GUIButton::GUIButton(Rectangle bounds, std::string label)
    : bounds(bounds), label(std::move(label)) {
}

bool GUIButton::Update(Vector2 mousePosition) const {
    return CheckCollisionPointRec(mousePosition, bounds) &&
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void GUIButton::Draw(Vector2 mousePosition) const {
    const bool isHovered = CheckCollisionPointRec(mousePosition, bounds);
    const Color backgroundColor = isHovered ? Color{72, 91, 118, 255}
                                            : Color{48, 61, 82, 255};
    const Color borderColor = isHovered ? GOLD : Color{150, 160, 178, 255};

    DrawRectangleRec(bounds, backgroundColor);
    DrawRectangleLinesEx(bounds, 2.0f, borderColor);

    constexpr float fontSize = 20.0f;
    Font fontSans = AssetManager::GetInstance().GetCustomFont("PixeloidSans");
    Vector2 textSize = MeasureTextEx(fontSans, label.c_str(), fontSize, 1.0f);
    float textX = bounds.x + (bounds.width - textSize.x) * 0.5f;
    float textY = bounds.y + (bounds.height - fontSize) * 0.5f;
    UIUtils::DrawText("PixeloidSans", label.c_str(), { textX, textY }, static_cast<UIUtils::FontSize>(fontSize), RAYWHITE);
}
