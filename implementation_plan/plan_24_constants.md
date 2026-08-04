# Implementation Plan: Centralized Configuration Refactoring

We will create a centralized `Constants.h` file and systematically refactor the entire codebase to eliminate hardcoded constants and unify the scaling logic behind the new `GLOBAL_SCALE` value of `1.5f`.

## Proposed Changes

### 1. `Constants.h` Creation
We will create `include/Core/Constants.h` containing:
```cpp
#pragma once
#include "raylib.h"

namespace Constants {
    // Window Configurations
    constexpr int SCREEN_WIDTH = 1280;
    constexpr int SCREEN_HEIGHT = 720;
    constexpr int GAME_WIDTH = 683; // Preserving internal resolution width for UI consistency
    constexpr int GAME_HEIGHT = 512; // Preserving internal resolution height for UI consistency
    inline const char* GAME_TITLE = "Voltron Mission - Galra Cypher";
    constexpr int TARGET_FPS = 60;

    // Scale & Transformation Constants
    constexpr float GLOBAL_SCALE = 1.5f; 
    constexpr float BASE_TILE_SIZE = 16.0f;

    // Gameplay & Weapon Offsets
    constexpr Vector2 KNIGHT_PROJECTILE_OFFSET = { 27.0f, 4.0f };
}
```

### 2. Search & Replace (Refactoring)
- **`main.cpp`:**
  - Remove the local anonymous namespace constants (`INITIAL_WINDOW_WIDTH`, `GAME_WIDTH`, `GLOBAL_SCALE`, `BASE_FPS`).
  - Update `InitWindow` to use `Constants::SCREEN_WIDTH`, `Constants::SCREEN_HEIGHT`, and `Constants::GAME_TITLE`.
  - Update `GameManager::GetInstance().UpdateTargetFPS(...)` to use `Constants::TARGET_FPS`.
- **`CameraManager.cpp`:**
  - Remove `extern const int GAME_WIDTH;`, `extern const int GAME_HEIGHT;`, and `extern const float GLOBAL_SCALE;`.
  - `#include "Core/Constants.h"` and replace hardcoded `683.0f` and `512.0f` scale divisors with `Constants::GAME_WIDTH` and `Constants::GAME_HEIGHT`.
  - Replace the zoom factor multiplication with `Constants::GLOBAL_SCALE` instead of the local hardcode.
- **`EnemyRangeState.cpp`:**
  - `#include "Core/Constants.h"` and replace the local hardcoded `27.0f` and `4.0f` calculations in `GetProjectileOrigin` with `Constants::KNIGHT_PROJECTILE_OFFSET`.

### 3. Rendering & Physics Checks
- By replacing the `GLOBAL_SCALE` in `CameraManager`, all world rendering (player, enemies, environments, collision boxes) implicitly respects the new `1.5f` zoom level immediately since the camera matrix inherently scales the view frustum.

## Verification Plan
### Automated Tests
- Build verification via `cmake --build build --config Debug` to ensure `Constants.h` links properly without redefinition errors.

### Manual Verification
- Launch the game to confirm the window spawns at exactly 1280x720.
- Verify the global camera scaling accurately reflects `1.5f` in the game scene.
- Ensure UI overlay elements remain proportionally constrained properly within the `GAME_WIDTH` letterbox calculations.
