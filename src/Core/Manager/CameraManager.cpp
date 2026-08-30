#include "Core/Manager/CameraManager.h"
#include "raymath.h"
#include <algorithm>
#include "Core/Constants.h"


/// Initializes the resources and collaborators required before this component can run.
void CameraManager::Initialize() {
    camera = { 0 };
    camera.target = { 0.0f, 0.0f };
    camera.offset = { 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    shakeOffset = { 0.0f, 0.0f };
    shakeTimeRemaining = 0.0f;
    shakeDuration = 0.0f;
    shakeMagnitude = 0.0f;
}

/// Starts a bounded screen-space shake. Repeated impacts strengthen/refresh an
/// existing shake without accumulating permanent camera displacement.
void CameraManager::StartShake(float duration, float magnitude) {
    if (!std::isfinite(duration) || !std::isfinite(magnitude) ||
        duration <= 0.0f || magnitude <= 0.0f) {
        return;
    }
    shakeDuration = std::max(shakeDuration, duration);
    shakeTimeRemaining = std::max(shakeTimeRemaining, duration);
    shakeMagnitude = std::max(shakeMagnitude, magnitude);
}

/// Advances the random shake sample and fades it toward zero over its lifetime.
void CameraManager::UpdateShake(float deltaTime) {
    if (shakeTimeRemaining <= 0.0f) {
        shakeOffset = { 0.0f, 0.0f };
        shakeDuration = 0.0f;
        shakeMagnitude = 0.0f;
        return;
    }

    shakeTimeRemaining = std::max(
        0.0f,
        shakeTimeRemaining - std::max(0.0f, deltaTime)
    );
    float strength = shakeDuration > 0.0f
        ? shakeTimeRemaining / shakeDuration
        : 0.0f;
    auto randomUnit = []() {
        return static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0f;
    };
    shakeOffset = {
        randomUnit() * shakeMagnitude * strength,
        randomUnit() * shakeMagnitude * strength
    };
}

/// Clamps the unshaken camera target to the active level bounds.
void CameraManager::ClampToLevel(
    Rectangle levelBounds,
    float screenW,
    float screenH
) {
    if (levelBounds.width <= 0.0f || levelBounds.height <= 0.0f ||
        camera.zoom <= 0.0f) {
        return;
    }

    float viewWidth = screenW / camera.zoom;
    float viewHeight = screenH / camera.zoom;
    float minX = levelBounds.x + viewWidth / 2.0f;
    float maxX = levelBounds.x + levelBounds.width - viewWidth / 2.0f;
    float minY = levelBounds.y + viewHeight / 2.0f;
    float maxY = levelBounds.y + levelBounds.height - viewHeight / 2.0f;

    camera.target.x = maxX < minX
        ? levelBounds.x + levelBounds.width / 2.0f
        : std::clamp(camera.target.x, minX, maxX);
    camera.target.y = maxY < minY
        ? levelBounds.y + levelBounds.height / 2.0f
        : std::clamp(camera.target.y, minY, maxY);
}

/// Updates camera.
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
    ClampToLevel(levelBounds, screenW, screenH);
    UpdateShake(deltaTime);
}

/// Smoothly centers and zooms the camera so the complete requested world area
/// remains visible. This is used for boss introductions and phase ceremonies.
void CameraManager::UpdateCinematicCamera(
    Rectangle focusBounds,
    float deltaTime,
    Rectangle levelBounds
) {
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());
    camera.offset = { screenW * 0.5f, screenH * 0.5f };

    constexpr float CINEMATIC_FRAME_MARGIN = 0.88f;
    float safeWidth = std::max(1.0f, focusBounds.width);
    float safeHeight = std::max(1.0f, focusBounds.height);
    float desiredZoom = std::min(
        screenW / safeWidth,
        screenH / safeHeight
    ) * CINEMATIC_FRAME_MARGIN;
    desiredZoom = std::max(1.0f, desiredZoom);

    Vector2 desiredTarget = {
        focusBounds.x + focusBounds.width * 0.5f,
        focusBounds.y + focusBounds.height * 0.5f
    };
    float blend = std::clamp(8.0f * std::max(0.0f, deltaTime), 0.0f, 1.0f);
    camera.zoom = Lerp(camera.zoom, desiredZoom, blend);
    camera.target.x = Lerp(camera.target.x, desiredTarget.x, blend);
    camera.target.y = Lerp(camera.target.y, desiredTarget.y, blend);

    ClampToLevel(levelBounds, screenW, screenH);
    UpdateShake(deltaTime);
}
