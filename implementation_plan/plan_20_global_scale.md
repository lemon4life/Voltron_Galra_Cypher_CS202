# Global Scaling and Rendering Overhaul

The current architecture draws all entities at their raw asset sizes. To increase their visual size proportionately without losing the pixel-perfect alignment or messing up the physics/collisions, we will implement a camera-based global scale factor rather than individually stretching every draw call. We will also fix the remaining visual bugs with the range enemy and the effects.

## Proposed Changes

### 1. Global Scale Factor (Camera Zoom)
- Add a new `constexpr float GLOBAL_SCALE = 3.0f;` constant inside `src/main.cpp` (or just directly inject it into the zoom interpolation logic).
- Multiply the target zoom calculation by `GLOBAL_SCALE`.
- By applying this at the camera level, the entire world (tiles, entities, projectiles, effects, and collision mathematics) is intrinsically scaled proportionately without any further edits to individual draw logic.

### 2. Range Enemy Fixes
- **Recoil Animation:** Set `kinematics.SetType(WeaponKinematicsType::Ranged);` inside the `EnemyRange` constructor. It was missing, causing `ApplyRecoil()` to return early without applying the kickback.
- **Bullet Spawn Point:** Modify `EnemyRange::GetProjectileOrigin()` in `src/Entities/EnemyEntities/EnemyRange.cpp` to return the position with a fixed rotational offset matching `(27, 4)` relative to the gun pivot (similar to Lance's logic).
- **Bullet Sizing:** With the camera-level global scale, the bullet will naturally scale proportionally with the rest of the game world.

### 3. Effect Adjustments
- Because we're implementing the `GLOBAL_SCALE` via the camera, the `sword_slash` will also be scaled correctly automatically without requiring custom width/height stretch modifiers.

## Verification Plan

### Automated Tests
- Run `cmake --build build --config Debug` to verify compilation.

### Manual Verification
- **Global Scaling:** Launch the game and visually confirm the world is significantly larger (zoomed in).
- **Recoil:** Engage a Range enemy and confirm the gun kicks back physically upon firing.
- **Bullet Offset:** Confirm the Range enemy bullet spawns precisely at the tip of the gun.
- **Effect Scaling:** Engage a Chaser or Diver and confirm their slash/stab animations match their weapon size visually.
