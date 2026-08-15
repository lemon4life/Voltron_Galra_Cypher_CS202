# Plan 18: Boss Fire-Punch Projectile

## Goal

Add the Boss Fire-Punch as an animated, lightly homing enemy projectile fired once at the end of every punch-animation loop. It must pass through walls, stop at the active room border, and use the map border as a safety limit.

This plan does not change the temporary Boss offense probabilities. The current `0/0/100` Chase/Spell/Punch values remain available for Punch testing.

## Confirmed Asset and Gameplay Data

- Asset: `assets/sprites/Enemy/Boss/Fire-Punch.png`.
- Sheet layout: four `64 x 64` frames in a `256 x 64` horizontal sheet.
- Projectile draw pivot/reference pixel: `(26, 29)` in each Fire-Punch frame.
- Projectile collision corners in frame space: `(17, 21)` and `(48, 39)`.
- Hand launch pixel: `(53, 7)` in each `Hand.png` frame.
- Existing hand rotation pivot: `(8, 7)`.
- Existing hand frame size: `54 x 14`.
- Default Fire-Punch speed: `400 px/s`, matching the current player bullet speed.
- Default maximum steering rate: `5 degrees/s`.
- The current Punch state runs ten punch loops, so one uninterrupted state produces ten Fire-Punch projectiles.

## Design Decisions

### 1. Use a specialized projectile class

Create `BossFirePunchProjectile`, derived from `Projectile`, rather than adding Boss-specific branches throughout the generic projectile code.

It will own:

- Current travel angle.
- Movement speed.
- Maximum turn rate in degrees per second.
- Four-frame looping animation state.
- A non-owning player target pointer used only for steering.
- A snapshot of the room bounds at spawn time.
- A snapshot of the whole-map bounds at spawn time.

It will override `Update()` and `Draw()` so its animation, rotation, collision rectangle, homing, and boundary lifetime are self-contained.

### 2. Keep the tuning values visible in `BossPunchState`

Add these explicitly named private fields to `BossPunchState`:

~~~cpp
float bulletSpeed = 400.0f;
float changeAngleDegreesPerSecond = 5.0f;
~~~

The state passes both values to the Boss fire helper. This keeps the two requested values easy to find and tune without exposing them globally.

### 3. Fire exactly at each punch-loop boundary

In the Punch phase, detect the transition from the last punch frame back to frame zero. At that boundary:

1. Spawn one Fire-Punch projectile.
2. Increment `completedPunches`.
3. Either begin the next punch loop or return to Idle after the tenth completed loop.

The projectile must be spawned before the final-loop state transition so the tenth punch also fires. Ready animation completion does not fire a projectile.

## Spawn Position and Rotation Calculation

The launch position must come from the rendered hand pose, not from the Boss center or collision box.

### 1. Hand-space launch offset

For the unflipped hand:

~~~text
hand pivot       = (8, 7)
launch pixel     = (53, 7)
launch offset    = (53 - 8, 7 - 7) = (45, 0)
~~~

When the hand is horizontally mirrored, mirror both points within the `54`-pixel-wide frame before subtracting them. This produces the corresponding local offset for the flipped hand and prevents the projectile from appearing on the wrong side.

### 2. World-space launch point

Reuse one Boss helper for both hand drawing and projectile spawning so both operations use the same:

- Boss body frame rectangle.
- Facing/mirroring rule.
- Body attachment pixel `(13, 43)`.
- Hand pivot `(8, 7)`.
- Aim rotation toward the player.

Calculate:

~~~text
launchWorld = handPivotWorld + Rotate(mirroredLaunchOffset, handRotation)
~~~

The Fire-Punch projectile position is `launchWorld`. Its draw origin is `(26, 29)`, so Fire-Punch pixel `(26, 29)` lands exactly on the hand's `(53, 7)` launch pixel.

### 3. Initial shot direction

At the instant of firing, copy the player's current world position and calculate:

~~~text
initialDirection = Normalize(playerPositionAtShot - launchWorld)
initialAngle     = atan2(initialDirection.y, initialDirection.x)
~~~

If the player reference is unavailable or the distance is effectively zero, use the hand's current forward direction instead.

## Limited Homing

After launch, the projectile may follow the live player position, but it must not rotate faster than the configured limit.

On each update:

1. Find the desired angle from the projectile pivot to the player's current position.
2. Wrap the signed difference into `[-180, 180]` so it always takes the shortest turn.
3. Calculate `maximumStep = changeAngleDegreesPerSecond * deltaTime`.
4. Clamp the signed difference to `[-maximumStep, +maximumStep]`.
5. Add the clamped value to the current angle.
6. Set velocity to `directionFromAngle * bulletSpeed`.

At the default `5 degrees/s`, the projectile bends gradually and cannot snap toward the player. If the target later becomes unavailable, it continues along its last heading.

## Animation and Drawing

- Register `Fire-Punch.png` in `AssetManager` under a clear Boss projectile key.
- Crop four horizontal `64 x 64` frames.
- Loop the animation continuously while the projectile exists.
- Put the animation frame duration in a named projectile constant; start with `0.10 s` per frame.
- Draw the frame rotated by the current travel angle with `(26, 29)` as the `DrawTexturePro` origin.
- Apply the same world scale convention used by current entity/projectile drawing. Convert all supplied sprite-space pixels through that scale once; do not mix scaled and unscaled coordinates.

## Collision Rectangle

The requested frame-local rectangle is:

~~~text
left   = 17
top    = 21
right  = 48
bottom = 39
width  = 31
height = 18
~~~

Treat the corners as geometric coordinates, giving a `31 x 18` rectangle. If later visual testing establishes that both endpoint pixels were intended to be inclusive, only the named collision-width/height constants need to change to `32 x 19`.

Because the sprite rotates, do not leave this collision rectangle axis-aligned in unrotated frame space. For every update:

1. Subtract the draw pivot `(26, 29)` from all four collision corners.
2. Rotate the four offsets by the projectile angle.
3. Add the projectile world pivot.
4. Take the minimum and maximum X/Y values to form the conservative world AABB expected by the existing collision system.

This stays compatible with `CheckCollisionRecs` without introducing a new oriented-box collision subsystem.

## Wall, Room, and Map Rules

### 1. Pass through walls

Add a small virtual projectile capability such as:

~~~cpp
virtual bool IgnoresWorldCollision() const { return false; }
~~~

Override it to return `true` only in `BossFirePunchProjectile`. In `GameManager::UpdateProjectiles()`, skip static-wall and destructible-environment collision handling for projectiles with this capability. Normal bullets retain their current behavior.

### 2. Room border destruction

Capture `LevelManager::GetCurrentRoomBounds()` when the projectile is created. Destroy the projectile as soon as its calculated world collision AABB touches or crosses any side of those captured bounds:

~~~text
collision.left   <= room.left
collision.right  >= room.right
collision.top    <= room.top
collision.bottom >= room.bottom
~~~

Using captured bounds prevents a later room-state transition from silently changing the projectile's permitted travel area.

### 3. Whole-map safety destruction

Also capture/check `LevelManager::GetLevelBounds()`. Destroy the projectile if its collision AABB is completely outside the map, or immediately if its position/collision values become non-finite. This is the safety fallback when a useful room boundary is unavailable or malformed.

Do not use the generic finite projectile lifetime for Fire-Punch. Its environmental lifetime is governed by the room border and map safety checks.

### 4. Character impacts

Preserve normal enemy-projectile interaction with the player, immunity, and parry systems. The Fire-Punch is destroyed after a successful player hit, as existing enemy bullets are. Here, "only disappear at the room border" is interpreted as the travel/environment rule: walls and ordinary world obstacles cannot remove it. If the intended combat rule is to pass through the player too, that should be added later with per-target hit tracking so it cannot damage the player every frame.

Use the Boss's existing damage value until a separate Fire-Punch damage value is requested.

## Files to Add

- `include/Entities/BossFirePunchProjectile.h`
- `src/Entities/BossFirePunchProjectile.cpp`

The recursive `src/*.cpp` CMake source discovery will pick up the new implementation automatically.

## Files to Modify

- `include/AI/EnemyState.h`
  - Add the two requested private tuning values to `BossPunchState`.
- `src/AI/BossState.cpp`
  - Fire once when every punch loop completes.
- `include/Entities/EnemyEntities/Boss.h`
  - Declare the Fire-Punch spawn/helper interface and, if needed, a shared hand-pose structure.
- `src/Entities/EnemyEntities/Boss.cpp`
  - Calculate the rendered hand launch transform and construct the specialized projectile.
- `include/Entities/Projectile.h`
  - Add the default world-collision capability query.
- `src/Core/Manager/GameManager.cpp`
  - Respect that capability for wall/destructible-environment checks while retaining player collision.
- `src/Core/Manager/AssetManager.cpp`
  - Load `Fire-Punch.png`.

## Implementation Order

1. Register and validate the four-frame Fire-Punch asset.
2. Add the specialized projectile with animation, pivot drawing, steering, and rotated collision AABB.
3. Add the opt-in world-collision bypass to the projectile pipeline.
4. Add captured room/map boundary destruction.
5. Extract or reuse the Boss hand-pose calculation and implement the exact `(53, 7)` to `(26, 29)` launch alignment.
6. Add the private speed/turn-rate values to `BossPunchState` and fire on every completed punch loop.
7. Confirm the temporary `0/0/100` offense probabilities are untouched.

## Validation Checklist

- Configure and build with `cmake -S . -B build` and `cmake --build build --config Release`.
- Confirm one Fire-Punch appears at the end of each of the ten punch loops, including the last loop.
- Confirm no shot is produced by the ready animation.
- Pause/capture a shot and verify Fire-Punch pixel `(26, 29)` begins at hand pixel `(53, 7)` for both Boss facing directions.
- Confirm all four Fire-Punch frames loop while it travels.
- Confirm its speed visually/numerically matches a player bullet at the default `400 px/s`.
- Move the player perpendicular to the shot and confirm the heading changes by no more than `5 degrees` over one second.
- Confirm it crosses solid walls and destructible scenery without disappearing or damaging those objects.
- Confirm its collision follows sprite rotation and corresponds to the `(17, 21)` to `(48, 39)` rectangle.
- Confirm touching any active room border destroys it.
- Test a fallback/no-room case and confirm leaving the whole map destroys it safely.
- Confirm player hit, immunity, and parry behavior still work.
- Confirm ordinary projectiles still collide with walls exactly as before.

## Acceptance Criteria

- Fire-Punch is a four-frame looping projectile.
- One projectile fires at the exact end of every Boss punch loop.
- Its initial pivot is aligned from `Hand.png` pixel `(53, 7)` to Fire-Punch pixel `(26, 29)`.
- It initially aims at the player's position at firing time and subsequently turns toward the live player by at most `5 degrees/s`.
- Its default `bulletSpeed` is the current player-bullet speed, and both speed and turn rate are private, visible fields in `BossPunchState`.
- Its effective collision derives from sprite rectangle `(17, 21)` to `(48, 39)` and follows projectile rotation.
- It ignores walls, stops at the captured room border, and cannot survive outside the map.
- Existing projectile behavior and the temporary Boss offense probability values remain unchanged.
