#pragma once

#include "Core/Constants.h"
#include "Core/Manager/CameraManager.h"
#include "raylib.h"

#include <algorithm>
#include <cmath>

struct VisibleTileRange {
    int minimumX = 0;
    int maximumX = -1;
    int minimumY = 0;
    int maximumY = -1;

    bool IsEmpty() const {
        return maximumX < minimumX || maximumY < minimumY;
    }
};

inline Rectangle GetVisibleWorldBounds(float padding = 0.0f) {
    Camera2D camera = CameraManager::GetInstance().GetRenderCamera();
    Vector2 first = GetScreenToWorld2D({ 0.0f, 0.0f }, camera);
    Vector2 second = GetScreenToWorld2D(
        {
            static_cast<float>(GetScreenWidth()),
            static_cast<float>(GetScreenHeight())
        },
        camera
    );
    float minimumX = std::min(first.x, second.x) - padding;
    float minimumY = std::min(first.y, second.y) - padding;
    float maximumX = std::max(first.x, second.x) + padding;
    float maximumY = std::max(first.y, second.y) + padding;
    return {
        minimumX,
        minimumY,
        maximumX - minimumX,
        maximumY - minimumY
    };
}

inline VisibleTileRange GetVisibleTileRange(
    Vector2 worldOffset,
    int width,
    int height,
    int paddingTiles = 2
) {
    if (width <= 0 || height <= 0) return {};
    Rectangle visible = GetVisibleWorldBounds(
        static_cast<float>(paddingTiles) * Constants::RENDER_TILE_SIZE
    );
    const float tileSize = Constants::RENDER_TILE_SIZE;
    VisibleTileRange range;
    range.minimumX = std::clamp(
        static_cast<int>(std::floor(
            (visible.x - worldOffset.x) / tileSize
        )),
        0,
        width - 1
    );
    range.maximumX = std::clamp(
        static_cast<int>(std::floor(
            (visible.x + visible.width - worldOffset.x) / tileSize
        )),
        0,
        width - 1
    );
    range.minimumY = std::clamp(
        static_cast<int>(std::floor(
            (visible.y - worldOffset.y) / tileSize
        )),
        0,
        height - 1
    );
    range.maximumY = std::clamp(
        static_cast<int>(std::floor(
            (visible.y + visible.height - worldOffset.y) / tileSize
        )),
        0,
        height - 1
    );
    return range;
}

inline bool IsWorldRectangleVisible(
    Rectangle bounds,
    float padding = Constants::RENDER_TILE_SIZE * 2.0f
) {
    return CheckCollisionRecs(bounds, GetVisibleWorldBounds(padding));
}
