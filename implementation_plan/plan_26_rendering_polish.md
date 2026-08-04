# Implementation Plan: Uniform Scaling & Map Rendering Polish

To resolve visual artifacts and ensure consistent global proportions, we need to enforce strict uniform scaling and correct the sub-pixel sampling errors in the tilemap rendering.

## Proposed Changes

### 1. Strict Uniform Scaling Enforcement
- **`src/Combat/MeleeAttackStrategy.cpp`**, **`src/Entities/EnemyEntities/EnemyChaser.cpp`**, **`src/Entities/EnemyEntities/EnemyDiver.cpp`**:
  - Remove all localized `scale = 2.0f` overrides that were manually boosting the sword slash and lance stab effects.
  - Replace them with `Constants::GLOBAL_SCALE`.
  - Ensure that `destRect.width` and `destRect.height` are mathematically derived exclusively from `sourceRect` dimensions multiplied by `GLOBAL_SCALE`.

### 2. Map Rendering Glitch Fixes
- **`src/Core/Manager/LevelManager.cpp`**:
  - **Texture Filtering:** Call `SetTextureFilter(tileset, TEXTURE_FILTER_POINT)` right after the tileset is loaded to prevent bilinear blending between adjacent tiles.
  - **Coordinate Flooring:** Wrap the calculated `destRec` coordinates in `std::floor` to eliminate sub-pixel positions that cause grid-line flickering when panning.
  - **UV Inset / Bleed Correction:** Modify the `sourceRec` construction to pad `x` and `y` by `+0.05f`, and shrink `width` and `height` by `-0.1f`. This micro-inset prevents the GPU from accidentally grabbing fragments from adjacent tiles on the texture atlas.

### 3. Static Attack Effects
- **`src/Entities/EnemyEntities/EnemyDiver.cpp` / `.h`**:
  - Add a `Vector2 staticEffectPos;` to the `EnemyDiver` class.
  - Record this static position the exact frame the enemy initiates the lunge attack (`!playingEffect` block).
  - Use `staticEffectPos` to draw the `Lance_stab_small.png` effect, completely unlinking the visual trail from the moving enemy's current position to give it a punchy, stationary afterimage look.

## Verification Plan
1. Check that the enemy sword slash and lance stab effects are correctly proportioned against their weapons natively.
2. Pan the camera across the tilemap and confirm the absence of dark border lines and corner dots between tiles.
3. Observe the `EnemyDiver` attack and verify the lance flash remains rooted in place while the enemy dashes forward.
