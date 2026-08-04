# Implementation Plan: Soul Knight Style Melee Refactor

We will refine Keith's `MeleeAttackStrategy` to achieve a precise "Soul Knight"-style procedural weapon swing and aligned slash effect.

## Open Questions
> [!WARNING]
> You mentioned both:
> 1. "trigger frame `0` of the `sword_slash` animation sequence on the exact frame the swing arc initiates."
> 2. "The sprite only appear when the sword is at about +30 degree"
> 
> **Clarification needed:** Do you mean the slash animation should start playing the moment the sword swings, but it visually *renders* rotated +30 degrees ahead of the sword? Or do you mean the slash effect shouldn't even become visible until the physical sword reaches +30 degrees in its arc? (In this plan, I assume you mean the texture rotation is offset by +30 degrees to match the sword's crescent path, but let me know!)

## Proposed Changes

### 1. Asset & Pivot Integration
- Revert `Keith.cpp` to pass `AssetManager::GetInstance().GetTexture("Sword_Slash_Small")` to the `MeleeAttackStrategy`.
- Adjust `MeleeAttackStrategy::Draw` so the `weaponTex` pivots at its handle (left edge). The origin will be set to `{0.0f, weaponTex.height / 2.0f}`.

### 2. Weapon Kinematics
- Modify `kinematics.ApplySwing()` calls to use a `120.0f` arc (from -60° to +60°).
- Ensure the swing duration perfectly matches the 3-frame animation of `Sword_slash_small`. (At `0.05s` per frame, the total swing duration will be `0.15s`).

### 3. Slash Rendering & Trig Alignment
- Update the slash rendering block to be drawn **AFTER** the weapon so it layers on top.
- Apply `BeginBlendMode(BLEND_ADDITIVE)` before drawing the slash and `EndBlendMode()` after.
- Use `cos` and `sin` to perfectly align the slash to the blade tip:
  ```cpp
  float rad = currentAngle * PI / 180.0f;
  float slashX = playerPos.x + cosf(rad) * slashOffsetDistance;
  float slashY = playerPos.y + sinf(rad) * slashOffsetDistance;
  ```
- Set the slash's origin to its center and pass `currentAngle` (plus the +30° offset if intended for rotation) so it rigidly locks to the blade's rotation during the ease-in-out interpolation.

### 4. Hitbox Sync
- Ensure the damage AABB is spawned during the active swing frames and properly applies hitstop via `GameManager::GetInstance().AddImpactEffect()`.

## Verification Plan
### Automated Tests
- Build verification via `cmake --build build --config Debug`.

### Manual Verification
- Select Keith and attack.
- Verify the sword pivots around its hilt.
- Verify the 3-frame additive slash trail accurately tracks the sword tip during the 120° arc.
