# Enemy rendering, physics, and Weapon Kinematics Overhaul

We will scale down the enemies and apply the fat pixel filter to their sprites. We will also introduce movement inertia for the enemies, and extract weapon procedural physics into a unified `WeaponKinematics` system shared by both players and enemies.

## Proposed Changes

### 1. Scale & Rendering (Fat Pixel Filter)
- **AssetManager:** Set `applyPointFilter = true` for the base Knight sprites (`Knight_Idle`, `Knight_Run`, `Knight_Down`) so they match the Paladin's fat pixel style.
- **Enemy Classes:** Instead of hardcoded rendering sizes (`48.0f`), we will update `EnemyDiver`, `EnemyChaser`, and `EnemyRange` to draw based on the source texture's `frameWidth` and `frameHeight` (similarly to `Paladin::Draw()`). This ensures the physical dimensions match the player exactly.

### 2. Movement & Physics
- **Speed Increase:** Increase base speeds (e.g., Range: 120, Diver: 160, Chaser: 150) to make them more threatening.
- **Inertia Implementation:** Add a `Vector2 currentVelocity` member to the base `Enemy` class. When the state machine instructs the enemy to move towards a target, we will interpolate `currentVelocity` toward the desired `moveDir * speed` using `Vector2Lerp`. We'll apply position updates using `currentVelocity * deltaTime` for smooth acceleration/deceleration.

### 3. Weapon Logic & Animation (Paladin Parity)
- Create a new component `WeaponKinematics` (`include/Combat/WeaponKinematics.h` and `src/Combat/WeaponKinematics.cpp`).
- **Functionality:** This class will manage procedural physics offsets.
  - **Gun (Ranged):** Exposes `ApplyRecoil(Vector2 aimDir)` and smoothly decays `recoilOffset` using an exponential falloff.
  - **Sword (Melee):** Exposes `ApplySwing(float duration)` to smoothly animate a rotational sweep arc (`angleOffset`) using a sine-wave or lerped inertia.
  - **Lance (Thrust):** Exposes `ApplyThrust(Vector2 aimDir, float duration)` to smoothly push the `positionOffset` outward and pull it back.
- **Integration:** 
  - Update `MeleeAttackStrategy`, `RangedAttackStrategy`, and `LaserAttackStrategy` to instantiate and delegate to `WeaponKinematics` for drawing offsets.
  - Update `EnemyDiver`, `EnemyChaser`, and `EnemyRange` to instantiate `WeaponKinematics`, trigger the respective procedural logic during attacks, and apply the returned offsets in their `Draw()` functions.

## Verification Plan
### Automated Tests
- Validate compilation (`cmake --build build`)

### Manual Verification
- Test all three enemies to confirm they render at the correct scale with crisp "fat pixel" filtering.
- Observe enemy movement for fluid acceleration/deceleration.
- Verify that both Paladins and enemies exhibit matching procedural weapon animations (recoil, swing arcs, and thrusts).
