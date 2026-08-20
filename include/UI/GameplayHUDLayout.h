#pragma once

#include "raylib.h"

#include <algorithm>
#include <cstddef>

namespace GameplayHUDLayout {
constexpr float EDGE_PADDING = 10.0f;
constexpr float HUD_MINIMAP_GAP = 16.0f;
constexpr float MINIMAP_SIZE = 100.0f;
constexpr float MAX_HUD_SCALE = 1.0f;
constexpr float HUD_TEXTURE_SOURCE_SCALE = 1.5f;
constexpr float PAUSE_BUTTON_WIDTH = 30.0f;
constexpr float HUD_BASE_HEIGHT = 48.0f;

struct Result {
    float scale = MAX_HUD_SCALE;
    float baseShellWidth = 226.0f;
    Rectangle pauseButtonBounds = {};
    Rectangle teamShellBounds = {};
    Rectangle playerHudBounds = {};
    Rectangle minimapBounds = {};
};

inline float GetBaseShellWidth(std::size_t teamSize) {
    if (teamSize == 2) return 354.0f;
    if (teamSize >= 3) return 482.0f;
    return 226.0f;
}

inline Result Calculate(Rectangle windowBounds, std::size_t teamSize) {
    Result result;
    result.baseShellWidth = GetBaseShellWidth(teamSize);
    result.minimapBounds = {
        windowBounds.x + windowBounds.width - EDGE_PADDING - MINIMAP_SIZE,
        windowBounds.y + EDGE_PADDING,
        MINIMAP_SIZE,
        MINIMAP_SIZE
    };

    float hudLeft = windowBounds.x + EDGE_PADDING;
    float baseHudWidth = PAUSE_BUTTON_WIDTH + result.baseShellWidth;
    float availableHudWidth = std::max(
        0.0f,
        result.minimapBounds.x - HUD_MINIMAP_GAP - hudLeft
    );
    float fitScale = baseHudWidth > 0.0f
        ? availableHudWidth / baseHudWidth
        : MAX_HUD_SCALE;
    result.scale = std::clamp(fitScale, 0.0f, MAX_HUD_SCALE);

    float hudHeight = HUD_BASE_HEIGHT * result.scale;
    float pauseWidth = PAUSE_BUTTON_WIDTH * result.scale;
    float shellWidth = result.baseShellWidth * result.scale;
    float hudTop = windowBounds.y + EDGE_PADDING;

    result.pauseButtonBounds = {
        hudLeft,
        hudTop,
        pauseWidth,
        hudHeight
    };
    result.teamShellBounds = {
        hudLeft + pauseWidth,
        hudTop,
        shellWidth,
        hudHeight
    };
    result.playerHudBounds = {
        hudLeft,
        hudTop,
        pauseWidth + shellWidth,
        hudHeight
    };
    return result;
}
}
