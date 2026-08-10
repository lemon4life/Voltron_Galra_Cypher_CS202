#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"
#include <algorithm>

namespace UIUtils {

    Vector2 MeasureText(const std::string& fontID, const std::string& text, FontSize size) {
        Font font = AssetManager::GetInstance().GetCustomFont(fontID);
        float fontSize = static_cast<float>(size);
        return MeasureTextEx(font, text.c_str(), fontSize, 1.0f);
    }

    void DrawText(const std::string& fontID, const std::string& text, Vector2 pos, FontSize size, Color color) {
        Font font = AssetManager::GetInstance().GetCustomFont(fontID);
        float fontSize = static_cast<float>(size);
        DrawTextEx(font, text.c_str(), pos, fontSize, 1.0f, color);
    }

    void DrawTextPro(const std::string& fontID, const std::string& text, Vector2 pos, Vector2 origin, float rotation, FontSize size, Color color) {
        Font font = AssetManager::GetInstance().GetCustomFont(fontID);
        float fontSize = static_cast<float>(size);
        ::DrawTextPro(font, text.c_str(), pos, origin, rotation, fontSize, 1.0f, color);
    }

    void DrawCenteredText(const std::string& fontID, const std::string& text, Vector2 centerPos, FontSize size, Color color) {
        Vector2 textSize = MeasureText(fontID, text, size);
        Vector2 pos = {
            centerPos.x - textSize.x / 2.0f,
            centerPos.y - textSize.y / 2.0f
        };
        DrawText(fontID, text, pos, size, color);
    }

    void DrawProgressBar(Rectangle bounds, float currentValue, float maxValue, Color bgColor, Color fillColor) {
        DrawRectangleRec(bounds, bgColor);
        if (maxValue > 0.0f) {
            float ratio = std::clamp(currentValue / maxValue, 0.0f, 1.0f);
            Rectangle fillRect = { bounds.x, bounds.y, bounds.width * ratio, bounds.height };
            DrawRectangleRec(fillRect, fillColor);
        }
    }

    void DrawSegmentedProgressBar(Rectangle bounds, float currentVal, float maxVal, int segments, Color bgColor, Color fillColor, Color dividerColor) {
        // Background
        DrawRectangleRec(bounds, bgColor);

        // Fill
        if (maxVal > 0.0f) {
            float ratio = std::clamp(currentVal / maxVal, 0.0f, 1.0f);
            Rectangle fillRect = { bounds.x, bounds.y, bounds.width * ratio, bounds.height };
            DrawRectangleRec(fillRect, fillColor);
        }
    }

    void DrawPanel(Rectangle bounds, Color color) {
        DrawRectangleRec(bounds, color);
        DrawRectangleLinesEx(bounds, 2.0f, LIGHTGRAY);
    }

    bool IsHovered(Rectangle bounds) {
        return CheckCollisionPointRec(GetMousePosition(), bounds);
    }

}
