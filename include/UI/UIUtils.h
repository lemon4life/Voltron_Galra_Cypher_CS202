#pragma once
#include "raylib.h"
#include <string>

namespace UIUtils {
    enum class FontSize { 
        SMALL = 16, 
        BODY = 24, 
        HEADER = 32, 
        TITLE = 64 
    };

    // Text Rendering
    Vector2 MeasureText(const std::string& fontID, const std::string& text, FontSize size);
    void DrawText(const std::string& fontID, const std::string& text, Vector2 pos, FontSize size, Color color);
    void DrawTextPro(const std::string& fontID, const std::string& text, Vector2 pos, Vector2 origin, float rotation, FontSize size, Color color);
    void DrawCenteredText(const std::string& fontID, const std::string& text, Vector2 centerPos, FontSize size, Color color);

    // Components
    void DrawProgressBar(Rectangle bounds, float currentValue, float maxValue, Color bgColor, Color fillColor);
    void DrawPanel(Rectangle bounds, Color color = ColorAlpha(BLACK, 0.6f));
    bool IsHovered(Rectangle bounds);
}
