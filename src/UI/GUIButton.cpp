#include "UI/GUIButton.h"

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

    constexpr int fontSize = 20;
    const int textWidth = MeasureText(label.c_str(), fontSize);
    const int textX = static_cast<int>(bounds.x + (bounds.width - textWidth) * 0.5f);
    const int textY = static_cast<int>(bounds.y + (bounds.height - fontSize) * 0.5f);
    DrawText(label.c_str(), textX, textY, fontSize, RAYWHITE);
}
