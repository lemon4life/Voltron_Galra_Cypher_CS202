# EnemyRange Architecture

## Goal

Add a ranged enemy named `EnemyRange` with exactly three runtime states:

1. `EnemyIdleState` — reuse the existing default detection state.
2. `EnemyRangeChaseState` — move toward the player and decide when a valid
   shooting position has been reached.
3. `EnemyRangeShootingState` — stop moving, predict the player's movement, and
   create enemy projectiles.

The corrected class spelling `EnemyRangeChaseState` is used instead of
`EnemeyRangeChaseState`.

The shooting distance is not a constructor argument. It is a named constant in
`EnemyRange.cpp`, so every `EnemyRange` instance uses the same designed range.
The existing inherited `attackCooldown` is the only firing timer; no separate
shot interval or shot-cooldown field is added.

## State Flow

```text
EnemyIdleState
    | player enters detection range
    v
EnemyRangeChaseState
    | player is within shooting range
    | and a projectile-safe sight line exists
    v
EnemyRangeShootingState
    | player leaves shooting range
    | or the firing line becomes blocked
    v
EnemyRangeChaseState
    | player leaves disengage range
    v
EnemyIdleState
```

Both `EnemyRangeChaseState` and `EnemyRangeShootingState` decrement the same
inherited `attackCooldown`. Only the current state updates each frame, so the
timer is decremented exactly once per frame and continues naturally across a
state transition.

## Behavior Rules

| Current state | Condition | Behavior |
| --- | --- | --- |
| Idle | Player enters detection range | Change to chase state |
| Chase | Player is outside disengage range | End pathfinding and return to idle |
| Chase | Player is outside shooting range | Pathfind toward player |
| Chase | Player is in range but sight is blocked | Continue moving to expose a firing line |
| Chase | Player is in range and sight is clear | End pathfinding and change to shooting state |
| Shooting | Player leaves range | Change back to chase state |
| Shooting | Predicted firing path is blocked | Change back to chase state |
| Shooting | Attack cooldown is above zero | Track and predict the player without firing |
| Shooting | Attack cooldown reaches zero | Fire at the predicted position and reset the inherited cooldown |

The initial implementation does not retreat when the player gets too close.
Kiting or a minimum preferred range can be added as a separate behavior later.

## Proposed Changes

### 1. Add the two new state classes

**[NEW] `include/AI/EnemyRangeState.h`**  
**[NEW] `src/AI/EnemyRangeState.cpp`**

```cpp
class EnemyRangeChaseState : public ITypedEnemyState<EnemyRange> {
public:
    void Enter(EnemyRange* enemy) override;
    void Update(EnemyRange* enemy, float deltaTime) override;
    void Exit(EnemyRange* enemy) override;
};

class EnemyRangeShootingState : public ITypedEnemyState<EnemyRange> {
private:
    Vector2 previousPlayerPosition = { 0.0f, 0.0f };
    Vector2 estimatedPlayerVelocity = { 0.0f, 0.0f };
    bool hasPreviousPlayerPosition = false;

    Vector2 PredictTargetPosition(
        EnemyRange* enemy,
        Player* player,
        float deltaTime
    );
    void FireProjectile(EnemyRange* enemy, Vector2 targetPosition);

public:
    void Enter(EnemyRange* enemy) override;
    void Update(EnemyRange* enemy, float deltaTime) override;
    void Exit(EnemyRange* enemy) override;
};
```

Neither state receives the shooting distance through its constructor. Both
states receive `EnemyRange*` directly through `ITypedEnemyState<EnemyRange>` and
query the entity for its fixed range/configuration without `dynamic_cast`.

#### `EnemyRangeChaseState` responsibilities

- Decrease the inherited `attackCooldown`, clamped to zero.
- Return safely when the enemy or target is missing.
- Return to `EnemyIdleState` when the player exceeds the disengage distance.
- Ask `LevelManager` whether a projectile-sized clear line exists to the player.
- Change to `EnemyRangeShootingState` only when the player is within the fixed
  shooting range and the sight line is clear.
- Otherwise use `EnemyPathManager::GetNextMoveTarget()` and
  `GetLocalAvoidanceDirection()` to chase or reposition.
- Apply axis-separated X/Y collision rollback so the enemy slides along walls.
- Balance `StartPathFinding()` and `EndPathFinding()` during transitions.

#### `EnemyRangeShootingState` responsibilities

- Decrease the same inherited `attackCooldown`, clamped to zero.
- Stop enemy movement while the shooting state remains valid.
- Estimate the player's velocity from consecutive position samples.
- Predict an intercept position using player velocity and projectile speed.
- Recheck range and projectile-safe line of sight every frame.
- Return to `EnemyRangeChaseState` if the player leaves range or the predicted
  firing path becomes blocked.
- Create and register the enemy projectile directly when the inherited
  `attackCooldown` reaches zero.
- Reset that same cooldown to the configured attack interval after firing.

All shooting implementation belongs to `EnemyRangeShootingState`. There is no
`EnemyRange::ShootAtPlayer()` method.

### 2. Add the `EnemyRange` entity

**[NEW] `include/Entities/EnemyEntities/EnemyRange.h`**  
**[NEW] `src/Entities/EnemyEntities/EnemyRange.cpp`**

Create the entity as both an `Enemy` and an `EnemyPathFinding` agent:

```cpp
class EnemyRange : public Enemy, public EnemyPathFinding {
private:
    std::unique_ptr<EnemyRangeShootingState> shootingState;

public:
    EnemyRange(Vector2 position, Player* target);
    ~EnemyRange() override;

    void Update(float deltaTime) override;
    void Draw() override;

    EnemyRangeShootingState* GetShootingState();

    bool IsWithinShootingDistance(Vector2 targetPosition) const;
    bool IsBeyondDisengageDistance(Vector2 targetPosition) const;
    float GetProjectileSpeed() const;
    float GetProjectileLifetime() const;
    float GetProjectileRadius() const;

    void StartPathFinding();
    void EndPathFinding();
};
```

Do not add any of the following:

- A shooting-distance constructor parameter.
- A `maxShootDistance` instance field.
- A `shotInterval` instance field.
- A `shotCooldownRemaining` instance field.
- A `ShootAtPlayer()` entity method.

Define the shared design values inside `EnemyRange.cpp`:

```cpp
namespace {
    constexpr int RANGE_MAX_HEALTH = 70;
    constexpr float RANGE_SPEED = 120.0f;
    constexpr int RANGE_DAMAGE = 12;
    constexpr float RANGE_ATTACK_COOLDOWN = 1.0f;
    constexpr float RANGE_DETECTION_DISTANCE = 700.0f;
    constexpr float RANGE_DISENGAGE_DISTANCE = 900.0f;
    constexpr float RANGE_SHOOTING_DISTANCE = 300.0f;
    constexpr float RANGE_PROJECTILE_SPEED = 320.0f;
    constexpr float RANGE_PROJECTILE_LIFETIME = 2.0f;
    constexpr float RANGE_PROJECTILE_RADIUS = 5.0f;
    constexpr float MAX_PREDICTION_TIME = 1.0f;
}
```

The exact balance values may be tuned during manual testing, but their ownership
must remain in `EnemyRange.cpp`.

Constructor setup:

```cpp
EnemyRange::EnemyRange(Vector2 position, Player* target)
    : Enemy(
          position,
          target,
          RANGE_MAX_HEALTH,
          RANGE_SPEED,
          RANGE_DAMAGE,
          RANGE_ATTACK_COOLDOWN
      ) {
    idleState = std::make_unique<EnemyIdleState>(RANGE_DETECTION_DISTANCE);
    chaseState = std::make_unique<EnemyRangeChaseState>();
    shootingState = std::make_unique<EnemyRangeShootingState>();
    ChangeState(GetIdleState());
}
```

`IsWithinShootingDistance()`, `IsBeyondDisengageDistance()`, and the projectile
getters are backed by file-local constants. They do not introduce duplicate
per-instance cooldown or distance storage.

### 3. Use one inherited attack-cooldown timer

The existing `Enemy::attackCooldown` is the only cooldown value for this enemy.
Both active states use the same update rule:

```cpp
float cooldown = enemy->GetAttackCooldown();
if (cooldown > 0.0f) {
    enemy->SetAttackCooldown(std::max(0.0f, cooldown - deltaTime));
}
```

The chase state decrements it while approaching or finding sight. The shooting
state continues decrementing it while aiming. Therefore changing from chase to
shooting never restarts the timer and does not grant an immediate extra shot.

After `EnemyRangeShootingState::FireProjectile()` creates a shot, it calls the
base `Enemy::ResetAttackCooldown()`. `Enemy` stores the constructor-supplied
attack cooldown as its immutable reset value, so this restores the inherited
timer to `RANGE_ATTACK_COOLDOWN` without adding a second mutable cooldown.

### 4. Implement predictive shooting inside the shooting state

`EnemyRangeShootingState` owns the temporary tracking data required for player
prediction. On `Enter()`:

- Copy the player's current position to `previousPlayerPosition`.
- Clear `estimatedPlayerVelocity`.
- Set `hasPreviousPlayerPosition` according to whether a target exists.

Each update estimates velocity:

```cpp
if (deltaTime > 0.0f && hasPreviousPlayerPosition) {
    estimatedPlayerVelocity =
        (currentPlayerPosition - previousPlayerPosition) / deltaTime;
}
previousPlayerPosition = currentPlayerPosition;
hasPreviousPlayerPosition = true;
```

Smooth or clamp the estimated velocity to reject single-frame teleport spikes.
Prediction should solve or approximate the interception problem:

```text
relativePosition = playerPosition - projectileOrigin
find t where:
    length(relativePosition + playerVelocity * t)
        == projectileSpeed * t
predictedPosition = playerPosition + playerVelocity * t
```

Use the smallest positive solution. If no valid solution exists, fall back to
`distance / projectileSpeed`. Clamp `t` to `MAX_PREDICTION_TIME` so inaccurate
long-range estimates cannot create extreme aim points.

Immediately before firing:

1. Recalculate the projectile origin just outside the enemy bounding box.
2. Recalculate the predicted target position.
3. Verify line of sight from the projectile origin to that predicted position.
4. Normalize the firing direction, with a stable fallback if its length is zero.
5. Construct `Projectile(origin, velocity, lifetime, damage, true)` directly in
   `EnemyRangeShootingState`.
6. Register it through `GameManager::AddProjectile()`.
7. Call the inherited `Enemy::ResetAttackCooldown()`.

### 5. Add projectile-safe line-of-sight testing

**[MODIFY] `include/Core/Manager/LevelManager.h`**  
**[MODIFY] `src/Core/Manager/LevelManager.cpp`**

Add a read-only query:

```cpp
bool HasClearLineOfSight(
    Vector2 start,
    Vector2 end,
    float projectileRadius = 5.0f
) const;
```

Implementation requirements:

- Sample the segment at intervals no larger than 8 pixels.
- At each intermediate point, create a rectangle based on
  `projectileRadius`.
- Reuse `IsSolidCollision()` so full walls, partial-width walls, layer-two
  obstacles, and map boundaries follow existing collision rules.
- Skip the shooter origin and stop before entering the target bounding box.
- Return `false` on the first blocked probe.
- Use the predicted target position when validating an actual shot.

This makes the visibility result match the space required by the projectile,
instead of treating visibility as an infinitely thin ray.

### 6. Register the new enemy type

**[MODIFY] `include/Entities/Enemy.h`**

Add `RANGE` to `EnemyType`.

**[MODIFY] `src/Core/EntityFactory.cpp`**

Add the new entity without a range argument:

```cpp
case 'R':
    return new EnemyRange(position, player);
```

**[MODIFY] `src/Core/Manager/WaveManager.cpp`**

Optionally introduce range enemies in waves two through four. Wave one remains
chaser-only and wave five remains the boss wave.

### 7. Temporary visuals

Until a final sprite exists:

- Draw `EnemyRange` in a color distinct from chasers and the boss.
- Draw its health bar using the existing enemy convention.
- Do not draw prediction or sight lines during normal gameplay.
- Guard any range, predicted-point, or line-of-sight visualization behind a
  debug flag.

## State Pseudocode

### EnemyRangeChaseState

```text
decrease inherited attackCooldown

if enemy or target is missing:
    return

if target is beyond disengage distance:
    stop pathfinding
    change to idle
    return

if target is within fixed shooting distance
and current player sight line is projectile-safe:
    stop pathfinding
    change to shooting
    return

start pathfinding
get next path waypoint, falling back to player position
apply local avoidance
move X with collision rollback
move Y with collision rollback
```

### EnemyRangeShootingState

```text
decrease inherited attackCooldown

if enemy or target is missing:
    change to chase when safe
    return

update estimated player velocity
calculate predicted intercept position

if player is outside fixed shooting distance
or predicted projectile path is blocked:
    change to chase
    return

remain stationary

if inherited attackCooldown is zero:
    create projectile toward predicted position directly in this state
    register projectile with GameManager
    reset inherited attackCooldown to configured attack interval
```

## Important Edge Cases

- Player position is unchanged: prediction should reduce to a direct shot.
- Player and projectile origin coincide: use a stable fallback direction.
- `deltaTime` is zero: keep the previous velocity estimate without division.
- A frame spike produces an extreme velocity estimate: clamp or smooth it.
- The intercept quadratic has no positive solution: use the fallback lead time.
- The predicted point is behind a wall: do not fire; change back to chase.
- The player moves behind a wall during cooldown: recheck every frame.
- The player leaves range during cooldown: preserve the timer while returning to
  chase.
- The enemy dies while pathfinding: `EndPathFinding()` and observer cleanup must
  remain safe to call more than once.
- No path is found: remain collision-safe and retry on a later path update.

## Verification Plan

### Build Verification

- Build with `cmake --build build --config Release`.
- Confirm the recursive `src/*.cpp` CMake glob includes both new source files.

### Manual Gameplay Checks

1. Spawn `EnemyRange` outside its fixed shooting range and verify it chases.
2. Enter range with a clear path and verify chase changes to shooting.
3. Confirm the enemy remains stationary while shooting.
4. Move sideways at a steady speed and verify projectiles lead the player.
5. Stop moving and verify prediction returns to direct aiming.
6. Move behind a wall and verify the shooting state changes back to chase.
7. Stand in range behind a partial-width wall and verify no shot passes through.
8. Move repeatedly across the range boundary and confirm the cooldown does not
   reset or tick twice during state transitions.
9. Measure consecutive shots and confirm their interval matches the inherited
   attack cooldown.
10. Verify enemy projectiles damage the player, collide with walls, and expire.
11. Leave the disengage range and verify the enemy returns to idle.
12. Kill the enemy while chasing and verify no stale pathfinding pointer remains.

## Completion Criteria

- `EnemyRange` has exactly the default idle, new chase, and new shooting states.
- `EnemyRangeChaseState` owns movement and the decision to begin shooting.
- `EnemyRangeShootingState` owns prediction and projectile creation.
- `EnemyRange` has no `ShootAtPlayer()` method.
- `EnemyRange` does not duplicate `ResetAttackCooldown()`; it uses the inherited
  `Enemy` implementation.
- Shooting distance is not accepted by the entity constructor and is stored as a
  constant in `EnemyRange.cpp`.
- The inherited `attackCooldown` is the only mutable shooting timer.
- Chase and shooting states both decrement that timer, once per active frame.
- Predictive shots are blocked when the projectile-sized sight path is not clear.
- The project builds cleanly and passes the manual checks above.
