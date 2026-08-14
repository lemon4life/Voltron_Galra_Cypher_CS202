#pragma once

#include "raylib.h"
#include <cmath>

class CameraManager {
public:
    static CameraManager& GetInstance() {
        static CameraManager instance;
        return instance;
    }

    void Initialize();
    void UpdateCamera(Vector2 playerPos, Vector2 mouseWorldPos, float deltaTime, Rectangle levelBounds, bool isHitstop);
    
    Camera2D& GetCamera() { return camera; }
    Camera2D GetRenderCamera() const {
        Camera2D cam = camera;
        cam.target.x = std::floor(cam.target.x);
        cam.target.y = std::floor(cam.target.y);
        cam.offset.x = std::floor(cam.offset.x);
        cam.offset.y = std::floor(cam.offset.y);
        return cam;
    }

private:
    CameraManager() = default;
    ~CameraManager() = default;

    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    Camera2D camera;
};
