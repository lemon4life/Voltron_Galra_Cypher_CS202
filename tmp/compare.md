# `main` and `UI-dev` comparison

This document compares the resolved `UI-dev` merge result against `main` at
commit `f9295f0`. Files already supplied unchanged by `main` are not repeated.
Renamed audio assets, background art, enemy sprites, camera code, particle
code, and weapon-kinematics code are inherited from `main` and therefore do
not appear as `UI-dev`-only differences below.

## 1. Additional file

These files exist in the merged `UI-dev` branch but not in `main`.

| File | Purpose |
| --- | --- |
| `include/UI/GUIButton.h` | Declares a reusable hoverable and clickable text button so menus do not duplicate input and drawing logic. |
| `src/UI/GUIButton.cpp` | Implements button hit-testing, click detection, hover colors, borders, and centered labels. |
| `include/UI/GUISlider.h` | Declares a reusable slider with a label and normalized `0.0`-to-`1.0` value. |
| `src/UI/GUISlider.cpp` | Implements slider dragging, clamping, value updates, and rendering for runtime volume controls. |
| `include/UI/PauseMenu.h` | Declares the pause overlay and its Resume, Back to Main Menu, Quit, sound-volume, and music-volume actions. |
| `src/UI/PauseMenu.cpp` | Implements the layered pause container and connects its sliders to `AudioManager`. |
| `tmp/compare.md` | Records the final organized difference between `main` and the merged `UI-dev` branch. |

## 2. Removed file

No files from `main` are removed by the merged `UI-dev` result.

Files that were previously absent from the old `UI-dev` tip, such as the camera
manager, particle manager, weapon-kinematics files, new visual assets, and
reorganized audio assets, are retained from `main`.

## 3. File change

| File | Which part changed | Why |
| --- | --- | --- |
| `AI Usage Note/AI Usage Notes - Week 7 - Phuc Khanh.md` | Adds entries for Chaser/Ranger behavior, reusable menu controls, and layered Hub/gameplay pause handling. | Preserves the feature prompts, generated content, and validation notes recorded on `UI-dev`. |
| `include/Core/Manager/AudioManager.h` | Adds persistent sound-effect and music volume fields, setters, and getters. | Allows the pause-menu sliders to inspect and change audio levels at runtime. |
| `src/Core/Manager/AudioManager.cpp` | Applies selected volume to existing and newly loaded sounds/music and updates laser, footstep, and click paths to `main`'s reorganized audio folders. | Keeps `UI-dev` volume control working without breaking `main`'s new asset layout. |
| `include/Core/Manager/GameManager.h` | Stores the state that was active before pausing and declares pause, resume, paused-state, and render-state helpers. | A single `PAUSE` state can suspend either the Hub or gameplay and later resume the correct source state. |
| `src/Core/Manager/GameManager.cpp` | Implements the pause helpers, adapts them to `main`'s state names, and initializes hitstop/FPS/impact members explicitly. | Preserves `UI-dev`'s layered pause behavior while avoiding uninitialized runtime state after the merge. |
| `include/Entities/EnemyEntities/EnemyChaser.h` | Adds per-enemy aggro progress, a randomized required duration, range checks, and reset/update helpers. | Prevents the Chaser from attacking instantly and gives its close-range attack a readable preparation rule. |
| `src/AI/EnemyChaserState.cpp` | Requires both completed cooldown and completed proximity aggro before entering the charge state. | Separates "near the player" from "ready to attack," making Chaser behavior less abrupt. |
| `src/Entities/EnemyEntities/EnemyChaser.cpp` | Retains `main`'s sprite and weapon-kinematics rendering, adds a 40-unit aggro range and randomized 0.2-0.7 second buildup, and uses a 0.25-second charge duration. | Combines the newer enemy visuals with the behavior tuning developed on `UI-dev`. |
| `src/AI/EnemyRangeState.cpp` | Randomly chooses direct or predictive targeting, validates both lines of sight, and applies up to three degrees of aim error. | Makes ranged enemies less mechanically perfect while preserving predictive shooting. |
| `include/UI/MainMenu.h` | Adds a consumable quit request to `main`'s loading/slideshow menu. | Lets the main loop perform normal cleanup instead of terminating directly inside a UI component. |
| `src/UI/MainMenu.cpp` | Keeps `main`'s rich background, logo, loading, and animated-button menu; restores Enter-to-start and converts Exit Game into a quit request. | Preserves the newer presentation while retaining `UI-dev`'s keyboard accessibility and controlled shutdown behavior. |
| `include/UI/UIManager.h` | Adds `IsPauseButtonPressed()`. | Keeps input handling in the main update loop instead of changing game state during HUD drawing. |
| `src/UI/UIManager.cpp` | Makes the HUD pause button draw-only and exposes its click through `IsPauseButtonPressed()`; retains `main`'s modal helper rendering. | Avoids UI state mutations during rendering and allows the same pause logic to work in both Hub and gameplay. |
| `src/main.cpp` | Reconciles the two state loops: keeps `main`'s deferred asset loading, camera, particles, enemy visuals, and settings overlay; integrates `UI-dev`'s pause menu, volume controls, Hub pausing, previous-state rendering, safe menu return, quit handling, virtual-resolution mouse input, and demo shortcut. Effect updates now occur in update phases rather than draw phases. | Provides one coherent runtime loop containing both branches' features and resolves the conflicting menu/pause/state naming implementations. |
