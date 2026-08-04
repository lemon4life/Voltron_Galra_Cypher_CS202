# Implementation Plan: Separating Physics and Combat Hitboxes

To improve top-down movement and prevent characters' heads/shoulders from getting stuck on walls while preserving their combat hurtboxes, we will implement dedicated physics collision boxes.

## Proposed Changes

### 1. `GameObject` Base Class Update
- **`include/Entities/GameObject.h`:**
  - Add a new virtual method: `virtual Rectangle GetCollisionBox() const { return GetBoundingBox(); }`.
  - By default, it falls back to the old bounding box, meaning inanimate objects (like walls or projectiles) don't need changes unless desired.

### 2. Player (Paladin) Physics Hitbox
- **`src/Entities/Player/Paladin.cpp / .h`:**
  - Override `GetCollisionBox()`.
  - The current combat hurtbox (`GetBoundingBox()`) is `{ position.x - 8, position.y - 12, 16, 24 }`.
  - The new physics collision box will cover only the bottom 30%: `{ position.x - 8, position.y + 4, 16, 8 }`.

### 3. Enemy Physics Hitbox
- **`src/Entities/Enemy.cpp / .h`:**
  - Override `GetCollisionBox()`.
  - The current combat hurtbox is `{ position.x - size.x/2, position.y - size.y/2, size.x, size.y }`.
  - The new physics collision box will cover the bottom 30%: `{ position.x - size.x/2, position.y + size.y * 0.2f, size.x, size.y * 0.3f }`.

### 4. Updating Collision Queries
- **`src/AI/EnemyCollision.cpp`:**
  - Update `pathAccess.IsBlocked(enemy.GetBoundingBox())` to use `enemy.GetCollisionBox()`.
- **`src/Core/Manager/EnemyPathManager.cpp`:**
  - Update grid overlap logic to query `enemy->GetCollisionBox()`.
- **`src/Entities/Player/PlayerState.cpp` & `PlayerDashState.cpp`:**
  - Update `levelManager->IsSolidCollision(player->GetBoundingBox())` to use `player->GetCollisionBox()`.
- **`src/Entities/Player/Paladin.cpp`:**
  - Update `levelManager->IsSolidCollision` checks in `UpdatePosition` to use `GetCollisionBox()`.

## Verification Plan
1. Compile the game to ensure the new virtual method signatures match.
2. Verify that the player can walk closer to upper walls (overlapping their head on the wall tile) without getting blocked.
3. Verify that enemies navigate corners smoothly without snagging their upper bounds.
4. Verify that combat (sword slashes, bullets) still reliably connects with the full sprite bounds.
