# Character Sprite & Animation Overhaul

## Goal
Update the game's rendering logic to support the new 32x32 pixel, 4-frame character sprites (`Idle_Sheet.png` and `Run_Sheet.png`). Implement robust 360-degree weapon rotation anchored precisely to the character's body based on top-left offsets, accurately mirroring when aiming left.

## Proposed Changes

### 1. Sprite Data Structures
**[MODIFY] include/Entities/Player/Paladin.h**
- Refactor the `CharacterSprites` struct to only contain:
  - `Texture2D idle;`
  - `Texture2D run;`
  - `Texture2D weapon;`
- Remove the redundant `rest` and `battle` texture variations since we only have one set of animations now.

### 2. Paladin Animation & Rendering
**[MODIFY] src/Entities/Player/Paladin.cpp**
- **Constructor**: Initialize `numFrames(4)` instead of 12. Pass `sprites.idle` to the base `Character` constructor.
- **GetIdleTexture / GetRunTexture**: Simplify these to unconditionally return `sprites.idle` and `sprites.run` regardless of whether the game is in the HUB or in combat.
- **Bounding Box**: Update `GetBoundingBox()` to return `{ position.x - 8.0f, position.y - 12.0f, 16.0f, 24.0f }` for tight collisions on the 32x32 sprite.
- **Weapon Anchor**: Update `GetWeaponPivot()` to apply top-left corner mathematics to offset the weapon exactly 12px from the left/right visual edge and 22px down.

### 3. Asset Loading
**[MODIFY] src/main.cpp**
- Update the sprite loading paths for Lance and Keith to point to the new directory structure:
  - `assets/sprites/Lance/Idle_Sheet.png`
  - `assets/sprites/Lance/Run_Sheet.png`
  - `assets/sprites/Lance/Weapon_Static.png`

### 4. NPC Overhaul
**[MODIFY] src/Entities/NPC.cpp**
- Update `LoadTexture` to point to `assets/sprites/Allura/Idle_Sheet.png`.
- Change `numFrames` initialization from 12 to 4 to match the 4-frame idle animation loop.

## Verification Plan
### Automated Tests
- N/A

### Manual Verification
- Verify that Lance and Allura render correctly with 32x32 sprites looping smoothly over 4 frames.
- Equip the weapon and test 360-degree aiming. Ensure the weapon pivots cleanly around the accurately placed pivot coordinates.
- Walk against a wall to ensure the new 16x24 Bounding Box provides accurate collision detection without clipping.
