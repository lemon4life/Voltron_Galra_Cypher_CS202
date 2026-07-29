#pragma once

#include "raylib.h"

class CameraManager {
public:
    static CameraManager& GetInstance() {
        static CameraManager instance;
        return instance;
    }

    void Initialize();
    void UpdateCamera(Vector2 playerPos, Vector2 mouseWorldPos, float deltaTime, Rectangle levelBounds, bool isHitstop);
    
    Camera2D& GetCamera() { return camera; }

private:
    CameraManager() = default;
    ~CameraManager() = default;

    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    Camera2D camera;
};
