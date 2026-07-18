# Implementation Plan: Hunk's Piercing Laser System

This document outlines the proposed technical changes to implement Hunk's Piercing Laser weapon system, including mathematical raycasting, AABB intersection, stretched rendering, and integration into Slot 3 of the Team Manager.

## Proposed Changes

### Core / Manager
#### [MODIFY] `src/main.cpp`
- Create a new `CharacterSprites hunkSprites` configuration.
- Load `Hunk/Idle_Sheet.png`, `Hunk/Run_Sheet.png`, `Hunk/Weapon_Static.png`, `Hunk/Muzzle.png`, `Hunk/Beam.png`, and `Hunk/Beam_Impact.png`.
- Apply `SetTextureFilter(..., TEXTURE_FILTER_POINT)` to all of these textures to prevent pixel blurring when rotated.
- Instantiate `Hunk` and assign him to Slot 3 in `teamManager`, replacing `PlaceholderPaladin`.

### Entities
#### [NEW] `include/Entities/Player/Hunk.h` & `src/Entities/Player/Hunk.cpp`
- Create the `Hunk` class inheriting from `Paladin`.
- Initialize `currentWeapon` using a new `LaserAttackStrategy` configured with the loaded Hunk assets.

#### [MODIFY] `include/Entities/Player/Paladin.h`
- Update the `CharacterSprites` struct to include `Texture2D laserBody` and `Texture2D laserImpact` (if not already covered by the generic names). Since we added `muzzleFlash`, `bullet`, and `impact` earlier, I will reuse these fields:
  - `weapon`: `Weapon_Static.png`
  - `muzzleFlash`: `Muzzle.png`
  - `bullet`: `Beam.png`
  - `impact`: `Beam_Impact.png`

### Combat (Laser Attack Strategy)
#### [NEW] `include/Combat/LaserAttackStrategy.h`
- Inherit from `IAttackStrategy`.
- Store textures: `weaponTex`, `muzzleTex`, `beamTex`, `impactTex`.
- Store state variables: `float laserTimer`, `Vector2 laserEndPoint`, `Vector2 barrelTip`.

#### [NEW] `src/Combat/LaserAttackStrategy.cpp`
- **Attack()**: 
  - Set `laserTimer` to `0.15f` (the lifetime of the laser).
  - Calculate `barrelTip` using trigonometry.
  - Compute `laserEndPoint` via a raycasting stepping loop (stepping outward by 8 pixels at a time) up to a maximum distance. At each step, check for environment collisions using `GameManager::GetInstance().GetLevelManager()->IsSolidCollision(Rectangle)`.
  - Once `laserEndPoint` is found, run a custom 2D Line-AABB intersection algorithm between the ray segment (`barrelTip` to `laserEndPoint`) and every active Enemy bounding box retrieved from `GameManager`.
  - For every intersected enemy: apply damage and trigger `OnHitEnemy()` to fill Hunk's EX battery instantly.
- **Update()**:
  - Decrement `laserTimer` by `deltaTime`.
- **Draw()**:
  - Render the static weapon sprite rotated toward the aim vector.
  - If `laserTimer > 0`, calculate the current animation frame (0 or 1) based on the timer progress.
  - Draw `Muzzle.png` using the calculated frame centered at `barrelTip`.
  - Draw `Beam.png` using `DrawTexturePro()`, stretching its destination width to `Vector2Distance(barrelTip, laserEndPoint)` and rotating it by `aimAngle`.
  - Draw `Beam_Impact.png` using the calculated frame centered precisely at `laserEndPoint`.

## Verification Plan

### Automated Tests
- N/A

### Manual Verification
- Compile and run the game.
- Switch to Hunk (Slot 3).
- Aim and shoot the Piercing Laser.
- Verify the laser perfectly hits the nearest solid wall, no matter the angle, and renders the impact splash at that exact surface.
- Verify the `Beam.png` stretches properly between the barrel tip and the impact point.
- Ensure all enemies caught inside the line segment take damage and instantly fill the EX gauge.
- Validate that texture filtering keeps the pixels sharp when rotating the weapon and beam.
