# Implementation Plan: Keith Procedural Melee Refactor

We will refactor Keith's `MeleeAttackStrategy` and the `WeaponKinematics` system to abandon frame-by-frame character attack animations in favor of a procedural weapon swing with an independent, layered effect animation.

## Proposed Changes

### 1. Asset Pipeline
- Update `AssetManager.cpp` to queue and load `assets/sprites/Effects/Sword_slash_big.png` under the key `"Sword_Slash_Big"`.
- Modify `Keith.cpp`'s constructor. Instead of passing `sprites.attack1` and `sprites.attack2` (the old character animations), we will pass the new `"Sword_Slash_Big"` effect texture into the `MeleeAttackStrategy`.

### 2. Procedural Swing (`WeaponKinematics`)
- Update `WeaponKinematics::ApplySwing` to take a new boolean parameter `reverse`. 
- When `reverse` is true, the swing target and start angles will be inverted. This creates an alternating arc (Up to Down -> Down to Up).
- The existing easing function (sine-wave `(1.0f - cosf(t * PI)) / 2.0f`) is already an ease-in-out implementation and provides the desired momentum/inertia natively.

### 3. Attack Strategy (`MeleeAttackStrategy`)
- **Combo System:** Update the combo logic to explicitly alternate between `comboStep 1` and `comboStep 2` on consecutive inputs, rather than picking randomly.
- **Animation Sync:** Set the total attack duration to `0.35s` (7 frames × `0.05s` per frame) to match the new `Sword_slash_big` animation length. Pass `reverse = (comboStep == 2)` into `ApplySwing()`.
- **Render Order:** In `MeleeAttackStrategy::Draw()`, render the `activeTex` (slash effect) FIRST, and the `weaponTex` SECOND. This forces the slash effect behind the physical sword.
- **Slash Flipping:** When swinging backwards (`comboStep == 2`), we will vertically invert the slash effect's source rectangle so the arc's visual direction matches the blade's travel path.

## Verification Plan
### Automated Tests
- Build verification via `cmake --build build --config Debug`.

### Manual Verification
- Select Keith as the active Paladin.
- Click to attack and verify the sword procedurally rotates in a smooth, ease-in-out arc.
- Click rapidly to trigger a combo and verify the sword alternates its swing direction (Down-Up).
- Verify the blue `Sword_slash_big` effect plays behind the sword during the swing.
- Verify hitboxes still properly deal damage and trigger hitstop/parry mechanics on impact.
