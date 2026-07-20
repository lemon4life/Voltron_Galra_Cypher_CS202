# Upscale Rendering Pipeline for Rotatable Assets

The objective is to fix the "muggy" and distorted look of pixel art when rotated by upscaling the raw image 4x before converting it to a texture, and applying bilinear filtering. This retains the crisp pixel aesthetic naturally while allowing smooth sub-pixel rotation.

## User Review Required

> [!WARNING]
> Because the textures will be natively 4x larger in memory, all combat strategy logic that relies on `texture.width` or `texture.height` (e.g., barrel length calculation, origin offsets, and rendering destination rectangles) must be scaled down by `4.0f`. Please review the logic changes below to ensure no physics/hitbox logic gets broken.

## Proposed Changes

We will introduce a constant scale factor of `4.0f` for all rotatable assets. 

### Core/Main 

#### [MODIFY] src/main.cpp
- Create a helper function `LoadUpscaledTexture(const char* path, int scale)`:
  - `Image img = LoadImage(path);`
  - `ImageResizeNN(&img, img.width * scale, img.height * scale);`
  - `Texture2D tex = LoadTextureFromImage(img);`
  - `UnloadImage(img);`
  - `SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);`
  - Return `tex`.
- Replace `LoadTexture` with `LoadUpscaledTexture(..., 4)` for:
  - Lance: `weapon`, `muzzleFlash`, `bullet`, `impact`
  - Keith: `weapon`, `attack1`, `attack2`
  - Hunk: `weapon`, `muzzleFlash`, `bullet`, `impact`

### Combat Strategy Rendering

We must adjust the `DrawTexturePro` logic in all combat strategies to handle the 4x larger textures. The `source` rectangle will use the full, upscaled texture dimensions. The `dest` rectangle and `origin` will divide the dimensions by `4.0f` to scale the image back down to its original in-game size. Barrel length logic must also divide by `4.0f`.

#### [MODIFY] src/Combat/LaserAttackStrategy.cpp
- `float barrelLength = weaponTex.width / 4.0f;`
- Source rects use full width/height.
- Destination rects divide width/height by 4.
- Origins divide width/height by 4.

#### [MODIFY] src/Combat/MeleeAttackStrategy.cpp
- Adjust `dest` and `origin` dimensions for `weaponTex` and `activeTex` (the slash animations) by dividing by 4.
- `frameWidth` calculation: `(float)activeTex.width / 4.0f` for the source, but for dest it becomes `((float)activeTex.width / 4.0f) / 4.0f` (since it's a sprite sheet, wait, yes, we divide by 4 for the frames, and then divide by 4 for the upscale scale).

#### [MODIFY] src/Combat/RangedAttackStrategy.cpp
- `float barrelLength = weaponTex.width / 4.0f;`
- Adjust `dest` and `origin` dimensions for `weaponTex` and `muzzleFlashTex` by dividing by 4.

## Verification Plan

### Manual Verification
1. Run the game.
2. Ensure characters' weapons rotate smoothly towards the mouse cursor without distortion.
3. Verify that the weapon size relative to the character remains identical to the previous version.
4. Verify that lasers, bullets, and muzzle flashes spawn at the correct tip of the barrel and maintain their original intended scale.
