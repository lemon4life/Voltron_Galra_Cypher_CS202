# Implementation Plan: Lance's Punchy Gun Shooting System

This document outlines the proposed changes to implement the requested visual and mechanical updates for Lance's shooting mechanics, including recoil, muzzle flash, bullet texturing, and animated hit impacts.

## Proposed Changes

### Core / Manager
#### [MODIFY] `include/Core/Manager/GameManager.h`
- Add a new struct or class `ImpactEffect` to manage the visual animations of bullet impacts.
- Add `std::vector<ImpactEffect> activeEffects` to store these effects.
- Add methods `AddImpactEffect(Vector2 pos)` and `UpdateAndDrawEffects(float deltaTime)`.

#### [MODIFY] `src/Core/Manager/GameManager.cpp`
- In `UpdateProjectiles()`, when a player's projectile hits an enemy or a solid wall:
  - Spawn an `ImpactEffect` at the collision coordinates.
  - Trigger `TakeDamage` on the enemy.
  - Trigger `OnHitEnemy` on the active Paladin to fill the EX gauge.
- Implement the loop for `UpdateAndDrawEffects()` to run the 4-frame animation (12x9 pixel frames) and destroy them upon completion.

### Combat (Ranged Attack Strategy)
#### [MODIFY] `include/Combat/RangedAttackStrategy.h`
- Add `Texture2D muzzleFlashTex` and `Texture2D bulletTex`.
- Add variables for recoil: `Vector2 recoilOffset`.
- Add variables for muzzle flash: `float muzzleFlashTimer`.

#### [MODIFY] `src/Combat/RangedAttackStrategy.cpp`
- **Recoil**: In `Attack()`, calculate the opposite direction of the aim vector and apply a pushback offset to `recoilOffset`.
- **Muzzle Flash**: In `Attack()`, reset `muzzleFlashTimer`.
- **Update()**: Lerp `recoilOffset` back to `{0,0}` using `deltaTime`.
- **Draw()**:
  - Apply `recoilOffset` to the weapon's draw destination.
  - If `muzzleFlashTimer > 0`, calculate the barrel tip using trigonometry and draw the muzzle flash sprite there.
- **Bullet Trajectory**: Pass the `bulletTex` to the projectile.

### Entities (Projectile)
#### [MODIFY] `include/Entities/Projectile.h`
- Add `Texture2D texture` to the class properties.
- Overload the constructor to accept the texture without breaking backward compatibility.

#### [MODIFY] `src/Entities/Projectile.cpp`
- In `Draw()`, check if the texture is valid. If it is, use `DrawTexturePro` with proper rotation (calculated from velocity using `atan2`).

### Main Entry Point
#### [MODIFY] `src/main.cpp`
- Load the new assets (`Muzzle_Flash.png`, `Bullet.png`, `Bullet_Impact.png`).
- Set `TEXTURE_FILTER_POINT` on all these textures to prevent pixel blurring upon rotation.
