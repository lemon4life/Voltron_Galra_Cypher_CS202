# Implementation Plan: Auto-Aim System

This plan documents the implementation of the toggleable Auto-Aim system that assists players (especially those on keyboard) with targeting enemies, replacing raw cursor aiming with dynamic target locking.

## 1. Global State Toggle
**Goal:** Allow players to toggle Auto-Aim on and off.
**Implementation:**
- Added `extern bool isAutoAimEnabled;` to `include/Core/Constants.h` and initialized it to `false` in `src/Core/Constants.cpp`.
- Updated `SettingsMenu` to include a new UI toggle button (`autoAimToggleBounds`). Clicking it flips `Constants::isAutoAimEnabled` and plays a click sound.

## 2. Enemy Target Acquisition
**Goal:** Continuously find the best enemy target for the active player.
**Implementation:**
- In `src/main.cpp`, within the core gameplay loop, if `Constants::isAutoAimEnabled` is active, the game iterates over all active enemies in `GameManager::GetInstance().GetActiveEnemies()`.
- It calculates the distance between the `activePaladin` and each enemy.
- The enemy with the shortest distance is selected as the `bestTarget`.
- The game calls `activePaladin->SetLockedEnemy(bestTarget)`. If no enemies are nearby or if Auto-Aim is disabled, it calls `activePaladin->SetLockedEnemy(nullptr)`.

## 3. Paladin Aim Logic Override
**Goal:** Override the cursor-based targeting when Auto-Aim is enabled.
**Implementation:**
- In `src/Entities/Player/Paladin.cpp`, the `Update` method branches based on `Constants::isAutoAimEnabled`.
- **Auto-Aim OFF:** `targetAimAngle` is strictly calculated using `atan2f` towards the mouse cursor's world coordinates (`aimTarget`).
- **Auto-Aim ON (with locked enemy):** `targetAimAngle` points directly at the `lockedEnemy->GetPosition()`.
- **Auto-Aim ON (without locked enemy):** The game falls back to 8-way directional aiming based on the current movement vector (`WASD`). If the player is running right, they aim right.
- This angle is then fed into the existing smooth angular interpolation (`currentAimAngle`) so the weapon swings dynamically toward the auto-locked target.

## 4. Crosshair Rendering Update
**Goal:** Visually indicate the locked target.
**Implementation:**
- In `src/main.cpp` rendering phase, if a `lockedEnemy` exists, the crosshair logic calculates `targetPos = activePaladin->GetLockedEnemy()->GetPosition()` and draws the crosshair over the locked enemy instead of following the raw mouse position. This provides immediate visual feedback that the player is locked on.
