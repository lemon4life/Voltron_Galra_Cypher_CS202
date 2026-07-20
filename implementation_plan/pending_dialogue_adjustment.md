# Dialogue Adjustment — High-Resolution Window Overlay

## Goal

Move dialogue drawing out of the fixed `683x512` game render texture and draw it
directly on the physical window after `DrawTexturePro()`. Preserve the original
`1366x1024` dialogue proportions while keeping the dialogue aligned with the
letterboxed game viewport at every supported window size.

This restores the original high-resolution dialogue pass without changing the
world, player, enemy, projectile, or HUD coordinate systems.

## Current Rendering Flow

The current frame is drawn in this order:

```text
683x512 render texture
  -> world and entities through Camera2D
  -> HUD in internal screen coordinates
  -> dialogue in internal screen coordinates

physical window
  -> scale the complete render texture into viewport.destination
```

Because dialogue is first rasterized at `683x512`, its font, borders, and
portraits are enlarged with the rest of the render texture. This keeps alignment
simple but loses the original high-resolution overlay quality.

## Target Rendering Flow

```text
683x512 render texture
  -> world and entities through Camera2D
  -> HUD in internal screen coordinates

physical window
  -> scale the render texture into viewport.destination
  -> draw dialogue directly inside viewport.destination
```

The dialogue remains visually attached to the game viewport, but it is
rasterized at the final window resolution rather than enlarged from `683x512`.

## Coordinate and Scale Rules

Use `viewport.destination` from `CalculateGameViewport()` as the dialogue's
physical drawing area.

The original dialogue layout was designed for a `1366x1024` screen, so calculate
its output scale from the viewport height:

```cpp
constexpr float DIALOGUE_REFERENCE_HEIGHT = 1024.0f;
float uiScale = viewport.height / DIALOGUE_REFERENCE_HEIGHT;
```

All dialogue measurements continue to use their original reference values and
are transformed only when drawing:

```text
physicalX = viewport.x + referenceX * uiScale
physicalY = viewport.y + referenceY * uiScale
physicalSize = referenceSize * uiScale
```

Horizontal anchoring must use `viewport.x` and `viewport.width`, not the full
window edges. Vertical anchoring must use `viewport.y` and `viewport.height`.
This prevents dialogue from moving into letterbox or pillarbox regions.

At the initial `1366x1024` window:

- `viewport.destination` is `{0, 0, 1366, 1024}`.
- `uiScale` is `1.0f`.
- Dialogue retains its original portrait, text, box, and border sizes.

At other window sizes, dialogue scales once from the original design directly
to the physical viewport. It is not passed through the low-resolution game
texture, so there is no double scaling.

## Planned Changes

### 1. Remove dialogue from the internal render-texture pass

**File:** `src/main.cpp`

Remove the current HUB dialogue call from between `BeginTextureMode(target)` and
`EndTextureMode()`:

```cpp
DialogueManager::GetInstance().Draw(GAME_WIDTH, GAME_HEIGHT);
```

Keep HUD rendering in this pass. HUD is part of the fixed-resolution gameplay
interface and is outside the scope of this adjustment.

### 2. Draw dialogue after the scaled game texture

**File:** `src/main.cpp`

Inside `BeginDrawing()`/`EndDrawing()`, draw in this order:

1. Clear the physical window to black.
2. Draw `target.texture` into `viewport.destination` with `DrawTexturePro()`.
3. If the current state is `GameState::HUB`, draw dialogue using the same
   `viewport.destination`.

Conceptual structure:

```cpp
DrawTexturePro(
    target.texture,
    sourceRec,
    viewport.destination,
    origin,
    0.0f,
    WHITE
);

if (state == GameState::HUB) {
    DialogueManager::GetInstance().Draw(viewport.destination);
}
```

Calling `Draw()` when dialogue is inactive remains safe because
`DialogueManager` already returns immediately in that case.

### 3. Make the dialogue API viewport-aware

**Files:**

- `include/Core/Manager/DialogueManager.h`
- `src/Core/Manager/DialogueManager.cpp`

Replace the ambiguous screen-size API:

```cpp
void Draw(int screenWidth, int screenHeight);
```

with a viewport-based API:

```cpp
void Draw(const Rectangle& viewport);
```

Passing the complete rectangle makes the drawing origin explicit and prevents
future code from assuming that the game always starts at window coordinate
`{0, 0}`.

### 4. Restore original reference-space measurements

**File:** `src/Core/Manager/DialogueManager.cpp`

Continue using the existing original measurements:

- `1024` reference height
- `20` margin
- `800` portrait height
- `250` dialogue-box height
- existing name-box, font, spacing, and border values

Calculate positions from the viewport rectangle:

- Left portrait: `viewport.x + margin`
- Right portrait: `viewport.x + viewport.width - portraitWidth - margin`
- Bottom-aligned values: `viewport.y + viewport.height - scaledOffset`
- Dialogue box width: `viewport.width - margin * 2`

Do not convert the dialogue to `GAME_WIDTH`/`GAME_HEIGHT` coordinates. The
purpose of this change is to draw directly in final physical pixels.

### 5. Clip the overlay to the game viewport

**File:** `src/main.cpp`

Wrap direct dialogue drawing in `BeginScissorMode()`/`EndScissorMode()` using a
rounded integer version of `viewport.destination`. This ensures portrait or
border pixels cannot appear in black letterbox regions because of rounding or
future layout changes.

The scissor rectangle must be created only from the already calculated viewport
and must never use the full window dimensions unless the viewport fills it.

### 6. Keep input and game logic unchanged

No dialogue input conversion is required because the current dialogue choices
use keyboard input rather than mouse hit-testing.

Do not change:

- dialogue tree parsing or progression;
- typewriter timing;
- selection controls;
- mission request handling;
- player, enemy, level, projectile, or camera drawing;
- HUD scale or HUD mouse conversion;
- render-texture resolution;
- viewport aspect-ratio calculations.

If mouse-controlled dialogue choices are added later, their window coordinates
must be tested directly against these viewport-transformed physical rectangles.

## Files to Modify During Implementation

- `src/main.cpp`
- `include/Core/Manager/DialogueManager.h`
- `src/Core/Manager/DialogueManager.cpp`

No new runtime source files or assets are required.

## Verification Plan

### Static verification

- Confirm there is no `DialogueManager::Draw()` call inside
  `BeginTextureMode(target)`/`EndTextureMode()`.
- Confirm dialogue is drawn after the final game `DrawTexturePro()` and before
  `EndDrawing()`.
- Confirm `DialogueManager::Draw()` receives `viewport.destination`.
- Confirm every dialogue edge anchor includes the viewport origin.
- Confirm scissor mode begins and ends within the same conditional block.
- Confirm gameplay and HUD drawing still use the internal `683x512` space.

### Build verification

From the repository root, run the commands specified by `AGENTS.md`:

```text
cmake -S . -B build
cmake --build build --config Release
```

If either command fails, stop and report the failure without attempting an
unrelated workaround.

### Manual gameplay verification

1. Launch at `1366x1024` and confirm dialogue matches its original size and
   placement.
2. Resize to a larger window with the same aspect ratio and confirm dialogue is
   sharper than the scaled gameplay texture.
3. Resize to a smaller window and confirm all dialogue elements remain inside
   the game viewport.
4. Test a wide window and confirm pillarboxes remain black with dialogue aligned
   to the game rather than the physical window edges.
5. Test a tall window and confirm letterboxes remain black with no portrait or
   border pixels leaking into them.
6. Verify left- and right-side speaker portraits.
7. Verify typewriter text, completed text, highlighted options, and option
   navigation.
8. Complete the introductory dialogue and confirm the mission transition still
   works.
9. Confirm player, enemy, level, HUD, camera, and aiming scale are unchanged.

## Risks and Mitigations

### Fractional viewport scale

Non-integer window sizes can produce fractional coordinates and font sizes.
Direct rendering will still be sharper than scaling the entire `683x512`
dialogue image, but borders may land between pixels. Use the viewport's existing
floating-point dimensions for layout and round only the scissor rectangle.

### Font source resolution

The font is loaded at size `32` and may still be enlarged for some dialogue text
sizes. This plan preserves the project's original font behavior and does not add
an unrelated font-loading refactor. A larger font atlas can be evaluated later
if text remains visibly soft.

### Draw-order regression

Dialogue must be drawn after the scaled game texture. Drawing it before
`DrawTexturePro()` would hide the overlay. Keep the final rendering order explicit
in `main.cpp`.

### Viewport leakage

Portraits or future animated dialogue elements could extend beyond the viewport.
Scissor mode confines all dialogue pixels to the game area.

## Completion Criteria

- Dialogue is drawn directly to the physical window after the scaled game
  texture.
- At `1366x1024`, dialogue uses its original `1.0f` reference scale.
- At other sizes, dialogue follows the centred game viewport and preserves the
  game aspect ratio.
- Dialogue never appears in letterbox or pillarbox regions.
- The world, entities, projectiles, HUD, aiming, and game coordinates are
  unchanged.
- Dialogue progression and mission transitions remain functional.
- The project configures and builds using the repository commands.

## Objective Assessment

This is the better rendering structure when dialogue clarity is more important
than making every UI element share the pixel-art render target. It keeps the
world and gameplay HUD predictable in a fixed virtual resolution while treating
dialogue as a presentation overlay rendered at the final output resolution.

The additional complexity is small: `DialogueManager::Draw()` receives one
viewport rectangle and `main.cpp` owns the final draw order. The important
management rule is that the viewport remains the single source of truth for
scale and offset. Dialogue should not independently calculate a second viewport
from raw window dimensions.

This plan intentionally does not move the HUD outside the render texture. Mixing
the two approaches is acceptable here because they serve different visual
purposes: the HUD belongs to the fixed-resolution gameplay presentation, while
dialogue contains large portraits and text that benefit from final-resolution
rendering.
