# Diver Enemy Architecture

## Goal

Add a new enemy type named `EnemyDiver`. It behaves like `EnemyChaser` while
idle and chasing, but uses two separate attack states:

1. `EnemyDiverReadyState` backs away from the player for `0.2` seconds.
2. `EnemyDiverLungingState` locks one direction and dives at high speed.

The Diver has higher health and base speed than `EnemyChaser`. Its chase and
ready states never damage the player. Only the lunging state can register an
attack.

## State Structure

`EnemyDiver` has four runtime states:

1. `EnemyIdleState` — reuse the existing default detection state.
2. `EnemyDiverChaseState` — chaser-style pathfinding and Ready-state decision.
3. `EnemyDiverReadyState` — fall backward for `0.2` seconds.
4. `EnemyDiverLungingState` — direction-locked dive followed by a `0.5`-second
   recovery wait.

All Diver-specific states use `ITypedEnemyState<EnemyDiver>` and receive
`EnemyDiver*` directly without `dynamic_cast`.

```cpp
class EnemyDiverChaseState : public ITypedEnemyState<EnemyDiver> {
public:
    void Enter(EnemyDiver* enemy) override;
    void Update(EnemyDiver* enemy, float deltaTime) override;
    void Exit(EnemyDiver* enemy) override;
};

class EnemyDiverReadyState : public ITypedEnemyState<EnemyDiver> {
private:
    float dTimer = 0.0f;

public:
    void Enter(EnemyDiver* enemy) override;
    void Update(EnemyDiver* enemy, float deltaTime) override;
    void Exit(EnemyDiver* enemy) override;
};

class EnemyDiverLungingState : public ITypedEnemyState<EnemyDiver> {
private:
    float dTimer = 0.0f;
    Vector2 lockedDirection = { 0.0f, 0.0f };
    bool isWaitingToChase = false;
    bool hasDamagedPlayer = false;

    void BeginRecovery(EnemyDiver* enemy);

public:
    void Enter(EnemyDiver* enemy) override;
    void Update(EnemyDiver* enemy, float deltaTime) override;
    void Exit(EnemyDiver* enemy) override;
};
```

Each new attack state owns a `dTimer`. The timer is decreased toward zero and
is the only counter used for that state's timed transition.

## State Flow

```text
EnemyIdleState
    | player enters detection range
    v
EnemyDiverChaseState
    | cooldown ready
    | player within short range
    | HasClearLineOfSight reports a clear body-width path
    v
EnemyDiverReadyState
    | dTimer counts down from 0.2 seconds
    v
EnemyDiverLungingState
    | dive timer ends or a wall stops the dive
    v
EnemyDiverLungingState recovery
    | dTimer counts down from 0.5 seconds while stationary
    v
EnemyDiverChaseState
```

## Behavior Rules

| State | Condition | Behavior |
| --- | --- | --- |
| Idle | Player enters sight distance | Change to chase |
| Chase | Player leaves off-sight distance | Return to idle |
| Chase | Ready conditions are not met | Continue chaser-style pathfinding |
| Chase | Cooldown ready, player close, line clear | Change to Ready |
| Ready | `dTimer > 0` | Move opposite the player's current position |
| Ready | Backward step touches a wall | Remain at the previous position and continue counting down |
| Ready | `dTimer <= 0` | Change to Lunging |
| Lunging | Dive is active | Move using the direction locked on state entry |
| Lunging | Player overlap occurs | Deal damage once without blocking or stopping movement |
| Lunging | Wall collision occurs | Stop the dive and begin recovery |
| Lunging | Dive timer reaches zero | Stop the dive and begin recovery |
| Lunging recovery | `dTimer > 0` | Remain stationary |
| Lunging recovery | `dTimer <= 0` | Reset attack cooldown and change to chase |

## Collision Interpretation

Ready and Lunging states ignore collisions with other enemies.

The Lunging state also has no physical collision response with the player: the
Diver is not pushed back, stopped, or redirected. A player bounding-box overlap
is retained only as a non-blocking attack trigger so the Diver can deal its one
allowed hit. `hasDamagedPlayer` prevents repeated damage while the bodies remain
overlapped.

Walls and map boundaries are the only objects that block Ready or Lunging
movement.

## Proposed Changes

### 1. Add the three typed Diver states

**[MODIFY] `include/AI/EnemyState.h`**  
**[NEW] `src/AI/EnemyDiverState.cpp`**

Forward-declare `EnemyDiver` and declare:

- `EnemyDiverChaseState`
- `EnemyDiverReadyState`
- `EnemyDiverLungingState`

#### `EnemyDiverChaseState`

Base movement on `EnemyChaserChaseState`:

- Decrease the inherited `attackCooldown`, clamped to zero.
- Start and end pathfinding through the existing observer system.
- Get path waypoints from `EnemyPathManager`.
- Apply local avoidance during ordinary chase movement.
- Use axis-separated X/Y wall collision rollback.
- Return to idle when the player exceeds the off-sight distance.
- Ask `EnemyDiver::CanEnterReadyState()` before moving.
- Change to Ready only when that query returns true.
- Never call `Player::TakeDamage()`.

Player overlap during chase is resolved without damage, matching the requirement
that attacks can register only during Lunging.

#### `EnemyDiverReadyState`

On `Enter()`:

- End pathfinding.
- Set `dTimer = READY_DURATION` (`0.2` seconds).

On every `Update()`:

1. Decrease `dTimer`, clamped to zero.
2. Read the player's current position.
3. Calculate
   `awayDirection = normalize(enemyPosition - playerPosition)`.
4. Propose movement using `awayDirection * READY_SPEED * deltaTime`.
5. Check only `LevelManager::IsSolidCollision()` against the Diver body.
6. If the proposed position touches a wall or boundary, restore the previous
   position and remain there. Do not cancel Ready and do not reset `dTimer`.
7. Ignore all other enemies and do not check player collision.
8. When `dTimer <= 0`, change to `EnemyDiverLungingState`.

The backward direction continues following the player's current position for
the full `0.2` seconds.

#### `EnemyDiverLungingState`

On `Enter()`:

- End pathfinding.
- Calculate `lockedDirection` once from the Diver's current position to the
  player's current position.
- Do not predict future player movement or calculate a dive endpoint.
- Set `dTimer = DIVE_DURATION`.
- Set `isWaitingToChase = false`.
- Set `hasDamagedPlayer = false`.

While the dive is active:

1. Never update `lockedDirection`.
2. Decrease `dTimer`, clamped to zero.
3. Move using `lockedDirection * DIVE_SPEED * deltaTime`.
4. Ignore every other enemy.
5. Use runtime wall collision only; do not precompute a dive corridor or target
   endpoint.
6. If a movement step touches a wall or map boundary, restore the previous
   position and call `BeginRecovery()`.
7. Use a non-blocking player overlap test only to apply one damage event. Do not
   stop or alter the dive after hitting the player.
8. When `dTimer <= 0`, call `BeginRecovery()`.

`BeginRecovery()`:

- Sets `isWaitingToChase = true`.
- Sets `dTimer = DIVE_RECOVERY_DURATION` (`0.5` seconds).
- Leaves the Diver stationary at its current valid position.

During recovery:

- Decrease the same Lunging-state `dTimer`, clamped to zero.
- Perform no movement and no attack checks.
- Keep the enemy in `EnemyDiverLungingState` until the wait finishes.
- When `dTimer <= 0`, call `Enemy::ResetAttackCooldown()` and change to chase.

On `Exit()`, clear the locked direction, recovery flag, hit flag, and `dTimer`.

### 2. Add the `EnemyDiver` entity

**[NEW] `include/Entities/EnemyEntities/EnemyDiver.h`**  
**[NEW] `src/Entities/EnemyEntities/EnemyDiver.cpp`**

```cpp
class EnemyDiverReadyState;
class EnemyDiverLungingState;
class LevelManager;

class EnemyDiver : public Enemy, public EnemyPathFinding {
private:
    std::unique_ptr<EnemyDiverReadyState> readyState;
    std::unique_ptr<EnemyDiverLungingState> lungingState;

public:
    EnemyDiver(Vector2 position, Player* target);
    ~EnemyDiver() override;

    void Update(float deltaTime) override;
    void Draw() override;

    EnemyDiverReadyState* GetReadyState();
    EnemyDiverLungingState* GetLungingState();

    bool CanEnterReadyState(LevelManager* levelManager) const;

    float GetReadyDuration() const;
    float GetReadySpeed() const;
    float GetDiveDuration() const;
    float GetDiveSpeed() const;
    float GetDiveRecoveryDuration() const;
    float GetCollisionClearanceRadius() const;

    void StartPathFinding();
    void EndPathFinding();
};
```

The previously proposed `IsBodyPathClear()` helper is removed. Ready-entry
clearance uses the existing `LevelManager::HasClearLineOfSight()` query instead.

### 3. Use `HasClearLineOfSight()` for the Ready transition

`EnemyDiver::CanEnterReadyState()` performs only the immediate transition
checks:

1. A player target exists.
2. The inherited attack cooldown is zero.
3. The player is within `DIVE_TRIGGER_DISTANCE`.
4. `LevelManager` exists.
5. The current body-width line from the Diver to the player is clear.

Use the existing LevelManager query:

```cpp
bool hasClearPath = levelManager->HasClearLineOfSight(
    GetPosition(),
    GetTarget()->GetPosition(),
    GetCollisionClearanceRadius()
);
```

`GetCollisionClearanceRadius()` should return half the larger Diver body
dimension. This makes the existing sampled line test approximate the clearance
required by the full body.

Do not calculate the future retreat endpoint, future dive endpoint, enemy
crowding, or a predicted player position. Ready and Lunging handle walls only at
runtime.

### 4. Define Diver constants

Keep balance values in `EnemyDiver.cpp`:

```cpp
namespace {
    constexpr int DIVER_MAX_HEALTH = 140;
    constexpr float DIVER_BASE_SPEED = 210.0f;
    constexpr int DIVER_DAMAGE = 30;
    constexpr float DIVER_ATTACK_COOLDOWN = 2.5f;
    constexpr float DIVER_SIGHT_DISTANCE = 40000.0f;
    constexpr float DIVER_OFF_SIGHT_DISTANCE = 40000.0f;
    constexpr float DIVE_TRIGGER_DISTANCE = 130.0f;

    constexpr float READY_DURATION = 0.2f;
    constexpr float READY_SPEED = 125.0f;
    constexpr float DIVE_SPEED = 700.0f;
    constexpr float DIVE_DURATION = 0.35f;
    constexpr float DIVE_RECOVERY_DURATION = 0.5f;

    constexpr Vector2 DIVER_SIZE = { 24.0f, 24.0f };
}
```

These starting values make the Diver faster and healthier than the current
chaser (`170` speed and `80` health).

Constructor setup:

```cpp
EnemyDiver::EnemyDiver(Vector2 position, Player* target)
    : Enemy(
          position,
          target,
          DIVER_MAX_HEALTH,
          DIVER_BASE_SPEED,
          DIVER_DAMAGE,
          DIVER_ATTACK_COOLDOWN
      ) {
    idleState = std::make_unique<EnemyIdleState>(DIVER_SIGHT_DISTANCE);
    chaseState = std::make_unique<EnemyDiverChaseState>();
    readyState = std::make_unique<EnemyDiverReadyState>();
    lungingState = std::make_unique<EnemyDiverLungingState>();
    enemyType = EnemyType::DIVER;
    size = DIVER_SIZE;
    ChangeState(GetIdleState());
}
```

### 5. Use runtime substeps for wall checks

“No precomputation” means the Diver starts moving immediately using its current
direction. High-speed Lunging movement should still use small runtime substeps
so it cannot pass completely through a wall between frames:

```text
frameDistance = DIVE_SPEED * deltaTime
maximumSubstep = min(bodyWidth, bodyHeight) / 2
substepCount = ceil(frameDistance / maximumSubstep)
substepDistance = frameDistance / substepCount
```

For every substep:

1. Save the current position.
2. Move using the fixed direction.
3. If `IsSolidCollision()` reports a wall, restore the saved position and begin
   recovery.
4. Otherwise keep the position.
5. Perform the non-blocking one-hit player overlap check.

Do not scan `LevelManager::GetEntities()` during Ready or Lunging.

### 6. Give every state a distinct color

`EnemyDiver::Draw()` selects a color from `GetCurrentState()`:

| State | Color | Meaning |
| --- | --- | --- |
| `EnemyIdleState` | `LIME` | Not actively chasing |
| `EnemyDiverChaseState` | `MAROON` | Pathfinding toward player |
| `EnemyDiverReadyState` | `ORANGE` | Backing away and preparing |
| `EnemyDiverLungingState` | `RED` | Diving or recovering |

Optionally use `DARKPURPLE` while `EnemyDiverLungingState` is in its recovery
wait, but this is a visual sub-phase rather than another state.

Keep the standard health bar visible in every state.

### 7. Register the Diver

**[MODIFY] `include/Entities/Enemy.h`**

Add `DIVER` to `EnemyType`.

**[MODIFY] `src/Core/EntityFactory.cpp`**

```cpp
case 'D':
    return new EnemyDiver(position, player);
```

**[MODIFY] `src/Core/Manager/LevelManager.cpp`**

Allow `D` as a legacy map entity marker.

**[MODIFY] `src/Core/Manager/WaveManager.cpp`**

Introduce one Diver in wave three and one Diver in wave four. Preserve the
existing boss-only fifth wave.

## State Pseudocode

### EnemyDiverChaseState

```text
decrease inherited attackCooldown

if no target or target is beyond off-sight range:
    stop pathfinding
    change to idle
    return

if enemy.CanEnterReadyState(levelManager):
    stop pathfinding
    change to ReadyState
    return

perform chaser-style pathfinding movement
resolve player overlap without damage
```

### EnemyDiverReadyState

```text
on enter:
    stop pathfinding
    dTimer = 0.2 seconds

on update:
    dTimer = max(0, dTimer - deltaTime)
    awayDirection = normalize(enemy position - current player position)
    previousPosition = enemy position
    move backward

    if new body position touches wall:
        restore previousPosition

    if dTimer is zero:
        change to LungingState
```

### EnemyDiverLungingState

```text
on enter:
    stop pathfinding
    lockedDirection = normalize(current player position - enemy position)
    dTimer = DIVE_DURATION
    isWaitingToChase = false
    hasDamagedPlayer = false

while not waiting:
    dTimer = max(0, dTimer - deltaTime)
    move in fixed-direction runtime substeps

    if wall touched:
        restore previous valid position
        isWaitingToChase = true
        dTimer = 0.5 seconds
        stop movement

    if player overlaps and hasDamagedPlayer is false:
        damage player once
        hasDamagedPlayer = true
        continue diving

    if dive dTimer is zero:
        isWaitingToChase = true
        dTimer = 0.5 seconds
        stop movement

while waiting:
    remain stationary
    dTimer = max(0, dTimer - deltaTime)

    if dTimer is zero:
        reset inherited attack cooldown
        change to ChaseState
```

## Important Edge Cases

- Diver and player share the same position when Ready updates: use a stable
  fallback away direction.
- Diver and player share the same position on Lunging entry: use the last valid
  direction or a stable default direction.
- A wall blocks backward Ready movement: remain in place while the `0.2`-second
  timer continues.
- Another enemy overlaps the Diver during Ready or Lunging: ignore it.
- The Diver overlaps the player during Ready: do not deal damage.
- The Diver overlaps the player during Lunging: damage once without stopping.
- The player remains overlapped for multiple frames: `hasDamagedPlayer` prevents
  repeated hits.
- A wall is hit during Lunging: restore the last valid position and immediately
  begin the `0.5`-second recovery.
- The lunge expires without hitting anything: begin the same recovery.
- Large `deltaTime`: runtime substeps prevent wall tunneling.
- The Diver dies in any state: remove it safely without stale pathfinding
  registration.

## Verification Plan

### Build Verification

- Build with `cmake --build build --config Release`.
- Confirm CMake includes `EnemyDiverState.cpp` and `EnemyDiver.cpp`.

### Manual Gameplay Checks

1. Spawn a Diver far from the player and verify chaser-style movement.
2. Confirm Diver health and chase speed exceed the chaser values.
3. Confirm Idle, Chase, Ready, and Lunging use visibly different colors.
4. Touch the Diver during Chase and Ready; verify no player damage.
5. Enter short range with a clear line and verify transition to Ready.
6. Verify Ready lasts `0.2` seconds and moves opposite the moving player.
7. Put a wall behind the Diver; verify it remains in place but still completes
   Ready after `0.2` seconds.
8. Verify Lunging locks its direction immediately on entry and never steers.
9. Put another enemy in the dive path; verify the Diver ignores it.
10. Stand in the dive path; verify one damage event without stopping the lunge.
11. Keep overlapping the Diver; verify no repeated damage during the same dive.
12. Put a wall in the dive path; verify the Diver stops at the last valid
    position.
13. Verify both wall-stopped and naturally completed dives wait `0.5` seconds
    before Chase.
14. Verify the attack cooldown resets when recovery completes.
15. Test with a large frame time and verify the Diver cannot pass through walls.

## Completion Criteria

- Ready and Lunging are two separate typed states.
- Each attack state owns and decrements a `dTimer`.
- Ready backs away for exactly `0.2` seconds.
- Ready ignores enemy/player collision and is blocked only by walls.
- A Ready wall collision leaves the Diver at its previous position without
  interrupting the timer.
- Lunging starts immediately with no endpoint or player-motion prediction.
- Lunging ignores other enemies and has no player collision response.
- Player overlap during Lunging can register at most one non-blocking attack.
- Only walls stop the active dive.
- The Diver waits `0.5` seconds after a dive before returning to Chase.
- `IsBodyPathClear()` is not added; `HasClearLineOfSight()` gates Ready entry.
- Every runtime state has a distinct debug color.
- The project builds cleanly and passes the manual checks above.
