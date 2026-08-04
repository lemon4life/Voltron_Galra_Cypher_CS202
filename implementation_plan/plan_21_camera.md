# Dynamic Camera Implementation Plan

We will extract the camera logic from `main.cpp` into a dedicated `CameraManager` that implements the dynamic "Soul Knight" aim-biased tracking and room constraints.

## Proposed Changes

### 1. Code Architecture
- **NEW HEADER:** `include/Core/Manager/CameraManager.h`
- **NEW SOURCE:** `src/Core/Manager/CameraManager.cpp`
- **Build System:** Since CMake uses `file(GLOB_RECURSE ... "src/*.cpp")`, the new `.cpp` file will automatically compile upon the next CMake generation. I will simply run the CMake build command for you. 

### 2. Core Mechanics (`CameraManager`)
- **Initialization:** Encapsulate the `Camera2D` struct and initialize default offsets.
- **`UpdateCamera(...)` Method:** This function will process:
  - **Aim-Biased Tracking:** Calculate `playerToMouse` vector. The camera's ideal target will be set to `PlayerPos + (playerToMouse * 0.25)`, capped at a `150.0f` maximum radius so the camera doesn't fly off-screen.
  - **Smooth Interpolation:** Use `raymath`'s `Lerp` to glide the camera to the ideal target. If the `hitstopTimer > 0.0f`, we will skip this lerp step to freeze the camera.
  - **Zoom / Global Scale:** Move the zoom calculation (`camera.zoom = Lerp(...)`) into the manager, ensuring it uses the window aspect ratio scale, hitstop modifier, and `GLOBAL_SCALE` factor.

### 3. World Constraints (Clamping)
- Calculate the camera's true visible viewport in world space (`screenWidth / camera.zoom`).
- After the lerp, clamp `camera.target.x` and `camera.target.y` against the level's bounding box (`LevelManager::GetLevelBounds()`). 
- If the level is smaller than the viewport, it will perfectly center the level on screen.

### 4. `main.cpp` Refactor
- Remove the local `Camera2D camera` struct.
- Instantiate `CameraManager cameraManager;`
- Pass the player position, `mouseWorld` position, `deltaTime`, `GetLevelBounds()`, and `GameManager` hitstop status into `cameraManager.UpdateCamera(...)`.
- Call `BeginMode2D(cameraManager.GetCamera())` during the drawing phase.

## Verification Plan
### Automated Tests
- Build verification via `cmake --build build --config Debug`.

### Manual Verification
- Walk around in the hub and gameplay states.
- Point the mouse away from the player and observe the camera slightly biasing toward the crosshair.
- Trigger an attack with hitstop (e.g., Lance firing/impact) and verify the screen accurately freezes.
- Walk into a corner of the map and verify the camera stops moving (clamped to the room boundary) rather than exposing black void areas.
