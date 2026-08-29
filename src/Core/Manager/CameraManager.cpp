#include "Core/Manager/CameraManager.h"
#include "raymath.h"
#include <algorithm>
#include "Core/Constants.h"


void CameraManager::Initialize() {
    camera = { 0 };
    camera.target = { 0.0f, 0.0f };
    camera.offset = { 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
}

void CameraManager::UpdateCamera(Vector2 playerPos, Vector2 mouseWorldPos, float deltaTime, Rectangle levelBounds, bool isHitstop) {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // 1. Screen Centering
    camera.offset = { screenW / 2.0f, screenH / 2.0f };

    // 2. Global Scale Integration & Zoom
    // Replicate main.cpp's scale logic
    // GAME_WIDTH and GAME_HEIGHT will be defined in main.cpp, but here we can just use 683 and 512
    float windowScale = std::min(screenW / (float)Constants::GAME_WIDTH, screenH / (float)Constants::GAME_HEIGHT);
    float hitstopZoom = isHitstop ? 1.1f : 1.0f;

    if (!isHitstop) {
        // Assume GLOBAL_SCALE = 3.0f is passed from somewhere or we define it here.
        // Actually we can just use 3.0f since it's hardcoded for now, or use the extern.
        camera.zoom = Lerp(
            camera.zoom,
            windowScale * hitstopZoom * Constants::GLOBAL_SCALE * 0.75f,
            std::clamp(15.0f * deltaTime, 0.0f, 1.0f)
        );
    }

    // 3. Aim-Biased Tracking
    Vector2 dir = Vector2Subtract(mouseWorldPos, playerPos);
    float distance = Vector2Length(dir);
    
    // Cap offset at 150 pixels distance
    float maxOffset = 150.0f;
    float offsetDist = std::min(distance * 0.25f, maxOffset);
    
    Vector2 idealTarget = playerPos;
    if (distance > 0) {
        idealTarget = Vector2Add(playerPos, Vector2Scale(Vector2Normalize(dir), offsetDist));
    }

    // 4. Smooth Interpolation
    if (!isHitstop) {
        // Fast enough to feel responsive, slow enough to be smooth
        float trackingFactor = std::clamp(10.0f * deltaTime, 0.0f, 1.0f);
        camera.target.x = Lerp(camera.target.x, idealTarget.x, trackingFactor);
        camera.target.y = Lerp(camera.target.y, idealTarget.y, trackingFactor);
    }

    // 5. World Constraints (Room Clamping)
    if (levelBounds.width > 0 && levelBounds.height > 0 && camera.zoom > 0) {
        float viewWidth = screenW / camera.zoom;
        float viewHeight = screenH / camera.zoom;

        float minX = levelBounds.x + viewWidth / 2.0f;
        float maxX = levelBounds.x + levelBounds.width - viewWidth / 2.0f;
        float minY = levelBounds.y + viewHeight / 2.0f;
        float maxY = levelBounds.y + levelBounds.height - viewHeight / 2.0f;

        // X Clamp
        if (maxX < minX) {
            // Level is smaller than viewport, center it
            camera.target.x = levelBounds.x + levelBounds.width / 2.0f;
        } else {
            camera.target.x = std::clamp(camera.target.x, minX, maxX);
        }

        // Y Clamp
        if (maxY < minY) {
            camera.target.y = levelBounds.y + levelBounds.height / 2.0f;
        } else {
            camera.target.y = std::clamp(camera.target.y, minY, maxY);
        }
    }
}
