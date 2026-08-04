# Enemy Rendering and Behavior Overhaul

We will migrate the Range, Driver (Diver), and Chaser enemies to use the new `Knight` base sprites, alongside unique weapons and hit effects. We will also implement weapon rotation and slow down enemy charging for debugging purposes.

## Proposed Changes

### 1. Asset Management (`AssetManager.h` & `AssetManager.cpp`)
- Queue the new Knight assets inside `QueueCharacterAssets`:
  - Base: `Knight.png`, `Knight_run-Sheet.png`, `Knight_down.png`
  - Range: `Knight_gun.png`, `Knight_gun_bullet.png`
  - Driver (Diver): `Knight_lance.png`, `assets/effects/Lance_stab.png`
  - Chaser: `Knight_sword.png`, `assets/effects/Sword_slash_small.png`
- Create an `EnemySprites` struct in `Enemy.h` to package these textures for easy injection into the Enemy classes.
- Expose methods `GetRangeSprites()`, `GetDiverSprites()`, `GetChaserSprites()` from the `AssetManager`.

### 2. Base Enemy Properties (`Enemy.h`)
- Add properties for animation state (e.g., `runFrameTime`, `currentRunFrame`).
- Track facing direction (`bool facingLeft`).
- Add weapon rotation properties (`float weaponAngle`).
- Add attack effect properties (`bool playingEffect`, `float effectTimer`, `int currentEffectFrame`).

### 3. Enemy-Specific Implementations

#### [MODIFY] src/Entities/EnemyEntities/EnemyDiver.cpp
- **Visuals**: Use `Knight` base sprites and `Knight_lance.png`.
- **Weapon Rotation**: Aim the lance towards the active target.
- **Attack Effect**: Upon entering `lungingState` or on a successful hit, trigger the 4-frame `Lance_stab.png` animation on the top rendering layer.
- **Debug**: Slow down `DIVER_BASE_SPEED` and `DIVE_SPEED`.

#### [MODIFY] src/Entities/EnemyEntities/EnemyChaser.cpp
- **Visuals**: Use `Knight` base sprites and `Knight_sword.png`.
- **Weapon Rotation**: Aim the sword towards the active target.
- **Attack Effect**: Trigger the 3-frame `Sword_slash_small.png` animation when the damage collision registers against the player.
- **Debug**: Slow down `CHASER_SPEED` and extend `CHASER_DAMAGE_CHARGE_DURATION`.

#### [MODIFY] src/Entities/EnemyEntities/EnemyRange.cpp
- **Visuals**: Use `Knight` base sprites and `Knight_gun.png`.
- **Weapon Rotation**: Aim the gun towards the active target.
- **Firing Logic**: When the shooting timer fires, instantiate a projectile struct/class representing `Knight_gun_bullet.png` travelling along the aiming vector. 
- **Debug**: Slow down `RANGE_SPEED`.

## Verification Plan
### Manual Verification
- Recompile the game.
- Observe the enemies spawning with correct sprites, proper animations (Idle, Run, Down).
- Confirm weapons rotate organically towards the player.
- Confirm attack effects trigger at the correct times on the topmost rendering layer.
- Ensure their movement logic matches the temporary debug slow-down.
