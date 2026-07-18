# Implementation Plan: Keith's Melee Combo System

This document outlines the proposed technical changes to implement a fluid, frame-perfect multi-hit combo state machine for Keith's sword attacks.

## Open Questions

> [!WARNING]
> I noticed that the `assets/sprites/Keith/` directory contains `Sword_2.png` rather than `Attack_2.png`. For consistency with your request, I will rename this file to `Attack_2.png` and use it as the second combo sequence. Let me know if you intended for me to use a different file.

## Proposed Changes

### Core Entities & Assets
#### [MODIFY] `include/Entities/Player/Paladin.h`
- Update the `CharacterSprites` struct to explicitly include `Texture2D attack1` and `Texture2D attack2` variables to hold the dynamic sword sheets.

#### [MODIFY] `src/main.cpp`
- Load `assets/sprites/Keith/Attack_1.png` and `assets/sprites/Keith/Attack_2.png`.
- Assign them to `keithSprites.attack1` and `keithSprites.attack2`.

### Combat (Melee Attack Strategy)
#### [MODIFY] `include/Combat/MeleeAttackStrategy.h`
- Redesign the class to support a combo state machine.
- Add new properties:
  - `Texture2D attack1Tex, attack2Tex`
  - `int comboStep` (0 = not attacking, 1 = Attack 1, 2 = Attack 2)
  - `float frameTimer`, `int currentFrame` (for slicing the 4-frame sprite sheets dynamically)
  - `bool inputBuffered` (to detect if the player pressed attack again during the combo window)

#### [MODIFY] `src/Combat/MeleeAttackStrategy.cpp`
- **Attack()**: 
  - If `comboStep == 0`, start Attack 1 (`comboStep = 1`), reset frames and timers.
  - If `comboStep == 1` and `currentFrame >= 2`, buffer the input (`inputBuffered = true`) so Attack 2 chains immediately after.
- **Update(float deltaTime)**:
  - Advance `frameTimer`. Every `X` seconds (based on Keith's attack speed), increment `currentFrame`.
  - **Hitbox Logic**: If `currentFrame == 1` or `currentFrame == 2` (the impact frames of the sword swing):
    - Spawn a dynamic AABB rectangle attached to Keith's facing side.
    - Check for collisions with active enemies using `GameManager::GetInstance().GetLevelEntities()`.
    - Deal damage, apply a knockback vector (using the aim direction), and trigger EX generation for Hunk/Lance/Keith via `Paladin::OnHitEnemy`.
  - **Combo Transition**: When `currentFrame >= 4`:
    - If `inputBuffered == true`, start Attack 2 (`comboStep = 2`, reset frames).
    - Else, end the attack state.
- **Draw()**:
  - Dynamically calculate the `Rectangle source` using `width / 4` and `currentFrame`.
  - Flip the source width if the player is facing left.
  - Center the sword swing relative to the player coordinates.

#### [MODIFY] `src/Entities/Player/Keith.cpp`
- Initialize `MeleeAttackStrategy` with the new combo textures.

## Verification Plan

### Automated Tests
- N/A

### Manual Verification
- Compile and run the game.
- Switch to Keith.
- Verify that a single click executes a 4-frame swing with a precise hitbox active ONLY during frames 2 and 3.
- Verify that double-clicking rapidly buffers a smooth transition directly into the second 4-frame animation swing.
- Ensure enemies hit by the swing take knockback away from Keith and EX is properly accumulated.

### Recent Updates Added
#### Alternating Combo Logic
- **`nextComboStep` Tracking**: A `nextComboStep` variable was added to `MeleeAttackStrategy` to continuously alternate between Attack 1 and Attack 2. If the player swings once and stops, their next attack will naturally be the opposite strike.

#### Idle Sword Rendering
- **Static Weapon Rotation**: When Keith is not mid-swing (`comboStep == 0`), the `MeleeAttackStrategy` now correctly falls back to rendering `Weapon_Static.png` aimed perfectly at the mouse cursor, matching the behavior of Lance and Hunk.
