# Refactor Observer into Capability-Based Level Access

## Goal

Replace the current single-listener `IEnemyObserver` relationship with small
capability interfaces implemented by `LevelManager`.

The design must support Enemy, DestructibleBox, and future GameObject types
without creating one LevelManager interface for every concrete object class.

The refactor covers these existing operations:

- Clear-line-of-sight queries.
- Enemy pathfinding registration.
- Enemy pathfinding removal.
- Safe deferred removal of enemies, boxes, and future level-owned objects.

## Core Design Rule

Create interfaces by capability, not by object type.

Do not create:

```cpp
IEnemyLevelAccess
IBoxLevelAccess
ITurretLevelAccess
IChestLevelAccess
```

Create reusable contracts instead:

```cpp
ILevelLineOfSightQuery
IEntityRemovalAccess
IEnemyPathAccess
```

A concrete object receives only the interfaces it needs.

| Object type | Required capabilities |
| --- | --- |
| Chaser | Removal and enemy path access |
| Range enemy | Removal, enemy path access, and line of sight |
| Diver | Removal, enemy path access, and line of sight |
| Boss | Removal; add other capabilities only when used |
| Destructible box | Removal |
| Turret | Removal and line of sight |
| Temporary level effect | Removal |

## Why This Replaces Observer

The current Enemy observer does not broadcast to an unknown or changing set of
listeners. `LevelManager` is the only receiver, and each call asks it to
perform a known service.

The new relationship is directional:

```text
GameObject or Enemy
    -> calls a narrow capability
    -> LevelManager performs the operation
```

There is no observer vector, subscription, unsubscription, or notification
loop.

## Proposed Files

### New file

- `include/Core/LevelAccess.h`

Keep the small related interfaces and construction bundle in one header to
avoid unnecessary file proliferation in this small project.

### Removed file

- `include/Core/IEnemyObserver.h`

### Modified files

- `include/Entities/Enemy.h`
- `src/Entities/Enemy.cpp`
- `include/Core/EnemyPath.h`
- `include/Core/EntityFactory.h`
- `src/Core/EntityFactory.cpp`
- `include/Core/Manager/LevelManager.h`
- `src/Core/Manager/LevelManager.cpp`
- `include/Core/Manager/EnemyPathManager.h`
- `src/Core/Manager/WaveManager.cpp`
- `include/Entities/EnemyEntities/Chaser.h`
- `src/Entities/EnemyEntities/Chaser.cpp`
- `include/Entities/EnemyEntities/EnemyRange.h`
- `src/Entities/EnemyEntities/EnemyRange.cpp`
- `include/Entities/EnemyEntities/EnemyDiver.h`
- `src/Entities/EnemyEntities/EnemyDiver.cpp`
- `src/AI/EnemyRangeState.cpp`

When the DestructibleBox plan is implemented, its constructor should consume
the shared removal capability instead of introducing a box-specific interface.

## 1. Define Reusable Capability Interfaces

Create `include/Core/LevelAccess.h`:

```cpp
#pragma once

#include "raylib.h"

class Enemy;
class GameObject;

class ILevelLineOfSightQuery {
public:
    virtual ~ILevelLineOfSightQuery() = default;

    virtual bool HasClearLineOfSight(
        Vector2 start,
        Vector2 end,
        float radius = 5.0f
    ) const = 0;
};

class IEntityRemovalAccess {
public:
    virtual ~IEntityRemovalAccess() = default;
    virtual void QueueRemoval(GameObject* object) = 0;
};

class IEnemyPathAccess {
public:
    virtual ~IEnemyPathAccess() = default;
    virtual void BeginPathFinding(Enemy* enemy) = 0;
    virtual void EndPathFinding(Enemy* enemy) = 0;
};

struct LevelAccessBundle {
    IEntityRemovalAccess* removal = nullptr;
    IEnemyPathAccess* pathFinding = nullptr;
    ILevelLineOfSightQuery* lineOfSight = nullptr;
};
```

The bundle exists only to transport dependencies through `EntityFactory`.
Concrete objects must receive individual capability pointers, not store the
entire bundle.

## 2. Let LevelManager Implement the Capabilities

Change the declaration to:

```cpp
class LevelManager
    : public IEntityRemovalAccess,
      public IEnemyPathAccess,
      public ILevelLineOfSightQuery {
```

Implement:

```cpp
bool HasClearLineOfSight(...) const override;
void QueueRemoval(GameObject* object) override;
void BeginPathFinding(Enemy* enemy) override;
void EndPathFinding(Enemy* enemy) override;
```

Map the old observer callbacks as follows:

| Old callback | New capability method |
| --- | --- |
| `OnEnemyDied(enemy)` | `QueueRemoval(enemy)` |
| `OnEnemyPathFind(enemy)` | `BeginPathFinding(enemy)` |
| `OnEnemyPathFindEnded(enemy)` | `EndPathFinding(enemy)` |

Keep the existing line-of-sight algorithm and EnemyPathManager calls unchanged
inside the renamed methods.

Add a helper for factories and spawners:

```cpp
LevelAccessBundle GetLevelAccessBundle() {
    return { this, this, this };
}
```

## 3. Generalize Deferred Removal

Change the pending-removal container from:

```cpp
std::vector<Enemy*> pendingRemoval;
```

to:

```cpp
std::vector<GameObject*> pendingRemoval;
```

`QueueRemoval()` must:

1. Ignore null pointers.
2. Verify the object is currently owned by `levelEntities`.
3. Avoid adding the same pointer more than once.
4. Never delete the object immediately.

`ProcessPendingRemovals()` must:

1. Find each queued object in `levelEntities`.
2. If it is an Enemy, remove it from EnemyPathManager.
3. Delete the object.
4. Erase it from `levelEntities`.
5. Clear the queue after processing.

This supports Enemy and Box removal through one lifecycle capability without
making boxes depend on enemy pathfinding methods.

## 4. Inject Only the Needed Capabilities

### Enemy base

The Enemy base needs deferred removal:

```cpp
IEntityRemovalAccess* removalAccess;
```

Its constructor receives that pointer. When health reaches zero for the first
time:

```cpp
if (removalAccess) {
    removalAccess->QueueRemoval(this);
}
```

Keep the existing one-time death/removal flag to prevent duplicate requests.

### EnemyPathFinding

Store only:

```cpp
IEnemyPathAccess* pathAccess;
```

Provide a protected setter or constructor initialization used by Chaser, Range,
and Diver.

Their `StartPathFinding()` and `EndPathFinding()` helpers call this capability
directly instead of looping through observers.

### Range and Diver

Store or receive an `ILevelLineOfSightQuery*` because these types currently
perform visibility checks.

Chaser does not receive line-of-sight access unless its behavior later needs
that query.

## 5. Pass Capabilities Through EntityFactory

Update the factory signature:

```cpp
static GameObject* CreateEntity(
    char type,
    Vector2 position,
    TeamManager* teamManager,
    const LevelAccessBundle& levelAccess
);
```

The factory receives the construction bundle but passes only the required
members:

```text
Chaser <- removal + pathFinding
Range  <- removal + pathFinding + lineOfSight
Diver  <- removal + pathFinding + lineOfSight
Boss   <- removal
NPC    <- no level capability unless required
Wall   <- no level capability
Box    <- removal
```

Update LevelManager text-map loading and WaveManager spawning to supply
`levelManager->GetLevelAccessBundle()`.

This keeps concrete enemies independent of the complete LevelManager class.

## 6. Remove IEnemyObserver

Remove from Enemy:

- The `IEnemyObserver.h` include.
- The observer vector.
- `AddObserver()`.
- `RemoveObserver()`.
- `NotifyEnemyDied()`.

Remove from LevelManager:

- `IEnemyObserver` inheritance.
- Observer registration in `AddEntity()`.
- Observer removal during cleanup.
- The three `OnEnemy...` overrides.

Remove unused includes from Chaser and EnemyPathManager, then delete
`include/Core/IEnemyObserver.h`.

Verify with:

```text
rg "IEnemyObserver|AddObserver|RemoveObserver|OnEnemyPathFind|OnEnemyDied" include src
```

The unrelated player/team `IObserver` and `ISubject` system is outside this
refactor and must remain unchanged.

## 7. Lifetime and Safety Rules

- LevelManager must outlive every object that stores one of its capability
  pointers.
- Capability pointers are non-owning and must never be deleted by entities.
- Removal must remain deferred until a safe LevelManager update boundary.
- Enemy destructors must not request removal again.
- Level clearing must empty the pending-removal queue before destroying owned
  entities.
- Pathfinding cleanup must occur before an Enemy is deleted.
- A missing optional capability must cause safe fallback behavior, not a crash.

## 8. Future Object Integration

When adding a new object, first list the services it actually requires.

Examples:

```cpp
DestructibleBox(
    Vector2 position,
    IEntityRemovalAccess* removal
);

Turret(
    Vector2 position,
    IEntityRemovalAccess* removal,
    ILevelLineOfSightQuery* lineOfSight
);
```

Create a new interface only when the object needs a genuinely new capability,
such as:

- `IEntitySpawnAccess`
- `ILevelCollisionQuery`
- `IAudioEventAccess`

Do not create an interface merely because the concrete object type is new.

## Verification Plan

### Static checks

- No `IEnemyObserver` references remain.
- Every constructor receives only the capabilities it uses.
- EntityFactory does not pass the complete LevelManager to entities.
- All removal requests enter one deduplicated deferred queue.
- Boxes and other non-enemy objects can use removal without path access.

### Build

Run:

```text
cmake --build build --config Release
```

If the documented build fails, stop and report it without forcing an alternate
build.

### Gameplay checks

1. Spawn Chaser, Range, Diver, and Boss enemies.
2. Confirm Chaser, Range, and Diver register/unregister pathfinding correctly.
3. Confirm Range and Diver line-of-sight behavior still respects walls.
4. Kill each enemy type and verify exactly one deferred removal.
5. Kill multiple enemies in one frame and check for iterator safety.
6. Reload a level while enemies are pathfinding.
7. Confirm WaveManager still recognizes cleared waves.
8. When boxes are implemented, destroy a box and verify it uses the same
   removal queue without entering EnemyPathManager.
9. Add a test object that uses only line of sight and verify it does not receive
   pathfinding or removal access.

## Completion Criteria

- Capability interfaces are organized by service, not concrete object type.
- LevelManager implements removal, pathfinding, and line-of-sight capabilities.
- Enemy and future objects receive only their required capability pointers.
- `IEnemyObserver` and its broadcast/subscription code are removed.
- Enemy and non-enemy objects share safe deferred removal.
- No object directly owns or deletes LevelManager.
- No immediate self-deletion occurs during combat or update iteration.
- The player/team observer system remains unchanged.
- All modified sources compile.

## Objective Assessment

For this project, capability interfaces are cleaner than both the existing
single-listener Observer and the proposed concrete facade.

### Benefits

- **Reusable:** A box and an enemy can share removal without sharing unrelated
  pathfinding methods.
- **Testable:** Each capability can be replaced by a small fake in unit or
  debug-harness tests.
- **Fast:** Calls are direct virtual dispatches; the cost is insignificant
  beside collision, pathfinding, rendering, and projectile processing.
- **Robust:** LevelManager retains ownership and performs deferred deletion.
- **Scalable:** New object types reuse existing capabilities rather than
  creating type-specific access classes.
- **Clear:** Constructor parameters document exactly what an object may request.

### Costs and risks

- Constructor and factory signatures become longer.
- LevelManager uses multiple inheritance from small pure interfaces.
- Too many tiny interfaces would make dependency wiring difficult.
- A construction bundle can become a service locator if objects store the full
  bundle instead of receiving individual capabilities.
- LevelManager can still become too large if every new service is implemented
  there permanently.

### Recommendation

Use the three capability interfaces now because they match the existing
responsibilities and support Enemy plus future boxes cleanly. Keep all three in
one small header, inject only the pointers each object uses, and keep deferred
removal centralized in LevelManager.

If the number of capabilities grows substantially, move their implementations
into dedicated managers such as EntityManager, EnemyPathManager, and
CollisionManager. Do not respond by creating one interface per object type or
one ever-growing facade.
