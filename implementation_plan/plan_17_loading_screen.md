# Loading Screen & UI Transition Implementation

This plan details the implementation of a chunk-based asset loader and a fluid, animated transition sequence from a loading screen directly into the Main Starter Menu UI.

## Proposed Changes

### 1. Asynchronous Chunked Loader

#### [MODIFY] `include/Core/Manager/AssetManager.h` & `src/Core/Manager/AssetManager.cpp`
- **New Feature**: Introduce a chunked loading pipeline.
- Define a `std::vector<std::function<void()>> loadTasks` (or similar simple struct) inside `AssetManager`.
- Add `QueueCharacterAssets()` which populates `loadTasks` with individual `LoadTexture2D` instructions rather than executing them instantly.
- Add `bool UpdateLoading(float& outProgress)` which executes 1-2 tasks per frame (to avoid blocking the main thread for long periods) and returns `true` when all tasks are complete. `outProgress` will return a value from 0.0 to 1.0.

### 2. Boot Sequence Refactoring

#### [MODIFY] `src/main.cpp`
- Currently, all heavy assets, Paladin initialization, and Level Managers boot up synchronously before the game loop starts. This causes the app window to freeze during startup.
- **Change**: 
  - Call `MainMenu mainMenu; mainMenu.Initialize();` immediately after initializing the window, particle shaders, and audio.
  - Instruct `AssetManager` to queue all heavy character assets.
  - Enter the `while (!WindowShouldClose())` loop immediately.
  - Prevent instantiation of the `TeamManager`, `LevelManager`, `WaveManager`, and Paladins until `AssetManager::UpdateLoading()` reports 100% completion (this allows the Main Menu to run at 60+ FPS while loading occurs in the background).
  - Once loading completes, instantiate the heavy game objects silently on the next frame.

### 3. MainMenu State Machine & Animations

#### [MODIFY] `include/UI/MainMenu.h`
- Introduce a state machine: `enum class MenuState { LOADING, TRANSITIONING, ACTIVE };`
- Add properties for UI animation tracking:
  - `MenuState currentState = MenuState::LOADING;`
  - `float logoYOffset` (for animating from top-center to final UI panel position).
  - `float logoXOffset`.
  - `float uiAlpha` (starts at 0.0, lerps to 1.0).
  - `float loadingIndicatorAlpha` and `float loadingIndicatorYOffset`.
  - `float transitionTimer`.

#### [MODIFY] `src/UI/MainMenu.cpp`
- **State: LOADING**:
  - `Update`: Calls `AssetManager::GetInstance().UpdateLoading(progress)`. The background slides and Ken Burns effect loop continuously. If loading completes, wait 0.5s (buffer for UX pacing) then switch to `TRANSITIONING`.
  - `Draw`: Renders background slides. Renders `Voltron_logo.png` centered horizontally near the top. Renders a sleek progress bar or text (e.g., "LOADING... 45%") at the bottom center.
- **State: TRANSITIONING**:
  - `Update`:
    - Loading indicator slides down and fades out.
    - Logo uses a smooth Lerp/Ease-out interpolation to glide from the top-center to its target anchor in the right-side gradient panel.
    - Right-side gradient overlay and textual buttons gracefully fade in and slide up from a slight Y-offset.
    - Once the sequence concludes (~1.2 seconds), switch to `ACTIVE`.
- **State: ACTIVE**:
  - `Update` & `Draw`: Identical to our current implementation (handles mouse hover input, scaling, and scene switching).

## User Review Required
> [!WARNING]
> Because Raylib handles OpenGL contexts on the main thread, the "chunked loading" handles 1 texture per frame. This successfully achieves asynchronous loading and a fluid loading screen, but the total load time may artificially inflate by a fraction of a second due to spreading the workload across frames. 

## Verification Plan

### Automated Tests
- Build via CMake and execute. Ensure no segfaults occur due to deferred dependency initialization in `main.cpp`.

### Manual Verification
- Observe the initial boot sequence: the window should open instantly, displaying the Ken Burns slideshow background, a top-centered logo, and a bottom progress bar.
- Upon 100% completion, verify the logo glides smoothly into the right side while the buttons fade up gracefully.
- Confirm input interactions are locked out until the transition sequence fully completes.
