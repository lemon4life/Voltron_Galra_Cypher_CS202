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

    static constexpr Color HP_GRADIENT_LEFT = {229, 242, 48, 255};
    static constexpr Color HP_GRADIENT_RIGHT = {84, 219, 99, 255};
    
    static constexpr Color EX_GRADIENT_LEFT = {30, 173, 240, 255};
    static constexpr Color EX_GRADIENT_RIGHT = {10, 251, 96, 255};
    
    static constexpr Color QUINT_GRADIENT_LEFT = {0, 255, 237, 255};
    static constexpr Color QUINT_GRADIENT_RIGHT = {157, 0, 198, 255};

    // Text Rendering
    /// Measures text.
    Vector2 MeasureText(const std::string& fontID, const std::string& text, FontSize size);
    /// Renders text.
    void DrawText(const std::string& fontID, const std::string& text, Vector2 pos, FontSize size, Color color);
    /// Renders text pro.
    void DrawTextPro(const std::string& fontID, const std::string& text, Vector2 pos, Vector2 origin, float rotation, FontSize size, Color color);
    /// Renders centered text.
    void DrawCenteredText(const std::string& fontID, const std::string& text, Vector2 centerPos, FontSize size, Color color);

    // Components
    /// Renders progress bar.
    void DrawProgressBar(Rectangle bounds, float currentValue, float maxValue, Color bgColor, Color fillColor);
    /// Renders segmented progress bar.
    void DrawSegmentedProgressBar(Rectangle bounds, float currentVal, float maxVal, int segments, Color bgColor, Color fillColor, Color dividerColor);
    /// Renders panel.
    void DrawPanel(Rectangle bounds, Color color = ColorAlpha(BLACK, 0.6f));
    /// Reports whether the hovered condition is satisfied.
    bool IsHovered(Rectangle bounds);
    /// Renders gradient pulse bar.
    void DrawGradientPulseBar(Rectangle bounds, float fillPercentage, Color leftColor, Color rightColor, bool isPulsing, bool applyGrayFilter);
    
    // UI Camera Helpers
    /// Creates centered ui camera.
    Camera2D CreateCenteredUICamera(float scale);
    /// Returns the current virtual mouse position.
    Vector2 GetVirtualMousePosition(const Camera2D& camera);
}
