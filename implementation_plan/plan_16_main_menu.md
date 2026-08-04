# Main Starter Menu UI

This plan outlines the architecture and tasks required to build a polished, interactive Main Starter Menu in Raylib, utilizing the provided background and animation assets, complete with a gradient side panel and animated UI buttons.

## Proposed Changes

### Core UI Component

#### [NEW] `include/UI/MainMenu.h`
Create a dedicated `MainMenu` class to manage the menu state and rendering logic autonomously, keeping `main.cpp` clean.
- Define a `StarEffect` struct holding properties like `position`, `scale`, `currentFrame`, `frameTimer`, and `startDelay` for the blinking background stars.
- Define a `MenuButton` struct holding text, bounding rect, `currentScale`, `currentXOffset`, and `colorLerp` for hover transitions.
- Expose methods `Initialize()`, `Update(float deltaTime)`, and `Draw(int screenWidth, int screenHeight)`.
- Method `HandleInput()` will check click intersections and manage GameState transitions or window closure.

#### [NEW] `src/UI/MainMenu.cpp`
Implement the logic defined in the header.
- **Initialization**: 
  - Load `assets/img/Background/Menu_bg.png`, `assets/img/Background/Voltron_logo.png`, and `assets/sprites/effects/star.png` directly or via `AssetManager`.
  - Instantiate a pool of randomized `StarEffect` objects across the screen bounds.
  - Setup 4 `MenuButton` instances ("Start Game", "Settings", "About us", "Exit Game").
- **Update Logic**:
  - Process star frame timers (7 frames at e.g. 10fps), obeying random start delays so they don't blink in unison.
  - Lerp button properties: If hovered, shift X-offset left by `-10.0f` to `-15.0f`, increase scale to `1.05f`, and push color toward `RAYWHITE`. If unhovered, lerp back to base values (Color: dull white, Scale: `1.0f`, Offset: `0.0f`).
- **Draw Logic**:
  - `DrawTexturePro` for the `Menu_bg.png` scaled to fit the full window resolution.
  - Loop and draw all active `StarEffect` frame slices.
  - Draw a horizontal gradient panel on the right side using `DrawRectangleGradientH()`, from low alpha black/dark gray on the left to solid on the right.
  - Draw the Logo texture in the upper half of the gradient panel.
  - Render the buttons using their lerped physical and color properties.

### Game Loop Integration

#### [MODIFY] `src/main.cpp`
Integrate the newly created `MainMenu` class.
- Instantiate `MainMenu mainMenu;` and call `mainMenu.Initialize();` during game startup.
- In the `GameState::MAIN_MENU` block inside the `while` update loop, execute `mainMenu.Update(deltaTime);`.
- We'll pass a reference to `bool shouldClose` (or let the menu return a specific action code) so "Exit Game" can properly terminate the application loop.
- In the `GameState::MAIN_MENU` block of the `BeginDrawing()` phase, remove the placeholder UI code and execute `mainMenu.Draw(GetScreenWidth(), GetScreenHeight());`.

## Verification Plan

### Automated Tests
- Build via CMake to ensure `MainMenu` links correctly without errors.

### Manual Verification
- Launch the application and observe the Main Menu background scaling seamlessly with window resize.
- Confirm stars spawn randomly and blink asynchronously.
- Verify the right-side gradient panel blends smoothly.
- Test button interactions: hovering should produce a smooth, satisfying scale-up and leftward shift alongside a color brighten.
- Click "Start Game" to ensure it transitions to the Hub/Battle state.
- Click "Exit Game" to ensure the application closes successfully.
