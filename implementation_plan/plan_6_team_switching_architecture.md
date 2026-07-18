# Implementation Plan: ZZZ Team-Switching Architecture

## Goal Description
Overhaul the game's core player architecture to support a 3-person tag-team mechanic inspired by Zenless Zone Zero (ZZZ). This involves replacing the singular `Player` class with a polymorphic `Paladin` structure and centralizing control in a new `TeamManager`.

## Proposed Changes

### Core Entity System
- [DELETE] `include/Entities/Player/Player.h`
- [NEW] `include/Entities/Player/Paladin.h`: Base class containing individual stats (`float hp`, `float exEnergy`).
- [NEW] `include/Entities/Player/Lance.h` & `Keith.h`: Derived classes with unique weapons and base stats.
- [NEW] `include/Entities/Player/PlaceholderPaladin.h`: Derived class for the 3rd team slot.

### Core Management System
- [NEW] `include/Core/Manager/TeamManager.h`: Central manager holding `std::vector<Paladin*> team` (max size 3) and handling shared team armor logic.
- [MODIFY] `src/Entities/Player/Paladin.cpp`: Route `TakeDamage()` calls to `TeamManager` to deduct shared armor before impacting individual HP.

### Dependency Refactoring
- [MODIFY] `src/Entities/Enemy.cpp` & `src/Core/Manager/WaveManager.cpp`: Replace all `Player*` dependencies with `TeamManager*`. Update enemies to actively target `TeamManager->GetActivePaladin()`.

## Verification Plan
1. **Manual Verification:** Run the game and press the **TAB** key. Verify that the active character instantly swaps between Lance, Keith, and the Placeholder, passing the active coordinates correctly. Verify enemies dynamically redirect their attacks to the active swapped character.
