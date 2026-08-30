#pragma once

#include "raylib.h"
#include <cmath>

// Design Pattern - Singleton:
// CameraManager is the sole owner of shared world/render camera state;
// GetInstance provides global access while copy construction is disabled.
class CameraManager {
public:
    /// Returns the process-wide singleton instance of this manager.
    static CameraManager& GetInstance() {
        static CameraManager instance;
        return instance;
    }

    /// Initializes the resources and collaborators required before this component can run.
    void Initialize();
    /// Updates camera.
    void UpdateCamera(Vector2 playerPos, Vector2 mouseWorldPos, float deltaTime, Rectangle levelBounds, bool isHitstop);
    
    /// Returns the current camera.
    Camera2D& GetCamera() { return camera; }
    /// Returns the current render camera.
    Camera2D GetRenderCamera() const {
        Camera2D cam = camera;
        // Snap zoom to nearest integer (min 1) — fixes elongated pixels and tile gaps.
        // A fractional zoom maps source texels to a non-integer number of screen pixels,
        // causing some pixels to appear wider/taller than neighbours.
        float zoom = fmaxf(1.0f, std::roundf(cam.zoom));
        cam.zoom = zoom;
        // Snap target to the nearest 1-screen-pixel boundary (= 1/zoom world units).
        // Using roundf(x*zoom)/zoom gives 1-px step movement instead of floorf(x)
        // which gave 2-px jumps at zoom=2 and felt laggy.
        cam.target.x = std::roundf(cam.target.x * zoom) / zoom;
        cam.target.y = std::roundf(cam.target.y * zoom) / zoom;
        // Floor offset (screen-space) to whole pixels
        cam.offset.x = std::floor(cam.offset.x);
        cam.offset.y = std::floor(cam.offset.y);
        return cam;
    }

private:
    /// Creates a CameraManager instance from the supplied configuration.
    CameraManager() = default;
    /// Releases resources owned by this CameraManager instance.
    ~CameraManager() = default;

    /// Creates a CameraManager instance from the supplied configuration.
    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    Camera2D camera;
};
