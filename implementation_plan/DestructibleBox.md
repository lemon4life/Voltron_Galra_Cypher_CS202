# Destructible Box Implementation Plan

## Goal

Add a destructible box item that occupies exactly one 32x32 map tile. The box
is a `GameObject` owned by `LevelManager`, blocks movement and line of sight
while alive, can be damaged by melee attacks and projectiles, and is removed
safely after its health reaches zero.

Place five boxes in open cells of `assets/levels/demo-big.txt` so the behavior
can be tested immediately from the demo map.

## Scope

This implementation includes:

- A new `DestructibleBox` game object under `Entities/Items`.
- A small shared damage interface for enemies and destructible items.
- Melee and projectile damage support for boxes.
- Dynamic collision, line-of-sight, spawn validation, and pathfinding support.
- Deferred box removal through `LevelManager`.
- An `X` marker for box placement in legacy text maps.
- Five boxes placed in the demo map.

This implementation does not include item drops, loot tables, inventory
behavior, box sprites, sound assets, or box placement in CSV maps.

## Runtime Behavior

| Event | Result |
| --- | --- |
| Player or enemy overlaps an intact box | Movement is blocked by its full 32x32 body |
| Enemy pathfinding reaches a box tile | The tile is treated as unwalkable |
| A clear-sight query crosses a box | The box blocks line of sight |
| Player melee attack hits a box | The box loses health once per swing |
| Player projectile hits a box | The box loses health and the projectile is destroyed |
| Enemy projectile hits a box | The box loses health and the projectile is destroyed |
| Box health reaches zero | It stops drawing and blocking, then is deleted safely |
| Level is cleared or reloaded | Remaining boxes are deleted with other level entities |

## Proposed Files

### New files

- `include/Combat/IDamageable.h`
- `include/Entities/Items/DestructibleBox.h`
- `src/Entities/Items/DestructibleBox.cpp`

### Modified files

- `include/Entities/Enemy.h`
- `include/Combat/MeleeAttackStrategy.h`
- `src/Combat/MeleeAttackStrategy.cpp`
- `include/Core/Manager/LevelManager.h`
- `src/Core/Manager/LevelManager.cpp`
- `src/Core/Manager/GameManager.cpp`
- `src/Core/EntityFactory.cpp`
- `assets/levels/demo-big.txt`

CMake already discovers `src/*.cpp` recursively with `CONFIGURE_DEPENDS`, so
the new source file does not require a manual CMake source-list edit.

## 1. Add a Shared Damage Interface

Create `include/Combat/IDamageable.h`:

```cpp
#pragma once

class IDamageable {
public:
    virtual ~IDamageable() = default;
    virtual void TakeDamage(int amount) = 0;
};
```

Update `Enemy` to inherit from `IDamageable` in addition to `GameObject` and
mark its existing `TakeDamage(int)` method as `override`.

This lets combat code damage enemies and boxes through one contract without
adding box-specific casts to every attack implementation. It does not change
enemy health, death notification, or removal behavior.

## 2. Add the Destructible Box GameObject

Create `DestructibleBox` as a concrete `GameObject` and `IDamageable`:

```cpp
class DestructibleBox : public GameObject, public IDamageable {
private:
    int health;
    int maxHealth;
    bool destroyed;

public:
    explicit DestructibleBox(Vector2 tileCenter);

    void Update(float deltaTime) override;
    void Draw() override;
    Rectangle GetBoundingBox() const override;
    void TakeDamage(int amount) override;

    bool IsDestroyed() const;
};
```

Keep the initial balance and layout constants in
`src/Entities/Items/DestructibleBox.cpp`:

```cpp
namespace {
    constexpr float BOX_WIDTH = 32.0f;
    constexpr float BOX_HEIGHT = 32.0f;
    constexpr int BOX_MAX_HEALTH = 100;
}
```

### Position and bounding-box convention

Text-map entities are created at tile centers. Store the box position as its
center and return:

```cpp
return {
    position.x - BOX_WIDTH / 2.0f,
    position.y - BOX_HEIGHT / 2.0f,
    BOX_WIDTH,
    BOX_HEIGHT
};
```

This covers one complete 32x32 block without spilling into adjacent cells.

### Damage and destruction

`TakeDamage()` should:

1. Ignore non-positive damage and calls made after destruction.
2. Subtract damage and clamp health to zero.
3. Set `destroyed = true` when health reaches zero.
4. Never delete the box directly from inside `TakeDamage()`.

Direct deletion is unsafe because melee and projectile systems may be
iterating `LevelManager::GetEntities()` when damage is applied.

### Initial visual

Do not require a new bitmap asset for the first version. Draw the box with:

- A brown 32x32 filled rectangle.
- A dark border.
- Two diagonal brace lines forming an `X`.
- A small health bar above the box only after it has taken damage.

`Draw()` should return immediately when `destroyed` is true.

## 3. Register the Text-Map Marker

Use uppercase `X` as the box marker. `B` is already reserved for Boss.

Update `EntityFactory::CreateEntity()`:

```cpp
case 'X':
    return new DestructibleBox(position);
```

Update the legacy text-map parser in `LevelManager::LoadLevel()` so `X` is
handled like other floor-based entities:

```cpp
else if (type == 'N' || type == 'E' || type == 'R' ||
         type == 'D' || type == 'B' || type == 'X') {
    tileID = 20;
    // Create the entity at the tile center.
}
```

The box marker therefore replaces a `.` visually; the floor tile remains under
the box after it is destroyed.

## 4. Add Box Collision to LevelManager

The existing `LevelManager::IsSolidCollision()` only checks CSV/text tiles.
Extend it so intact destructible boxes also count as solid objects.

Add an optional flag:

```cpp
bool IsSolidCollision(
    Rectangle box,
    bool includeDestructibleItems = true
) const;
```

The method should:

1. Preserve all existing tile and out-of-bounds checks.
2. When `includeDestructibleItems` is true, scan `levelEntities` for intact
   `DestructibleBox` objects.
3. Return true when the supplied rectangle overlaps a box bounding box.
4. Ignore boxes whose `IsDestroyed()` is true.

Existing movement, enemy wall checks, dive collision, line-of-sight queries,
and spawn validation continue calling the default form. They will therefore
treat boxes as solid without changes in every enemy state.

Projectile code must call `IsSolidCollision(projectileBox, false)` so a box is
not mistaken for an indestructible map wall before the projectile/entity
damage pass runs.

### Pathfinding

Update `LevelManager::IsWalkableTile()` to reject a tile occupied by an intact
box. Construct the queried tile's 32x32 rectangle and test it against box
bounding boxes after the existing tile-ID checks.

Once the box is destroyed and removed, the tile becomes walkable again during
the next pathfinding update.

## 5. Remove Destroyed Boxes Safely

Add a private `LevelManager::ProcessDestroyedItems()` helper. It should iterate
`levelEntities` with an iterator and erase only boxes for which
`IsDestroyed()` is true:

```text
for each level entity:
    if entity is a DestructibleBox and box.IsDestroyed():
        delete the box
        erase the vector entry
    else:
        continue
```

Call this helper at the beginning and end of `UpdateLevel()`, alongside the
existing enemy pending-removal processing.

Keep enemy observer/pathfinding removal unchanged. Boxes are not enemies and
must never be registered with `EnemyPathManager` or counted by `WaveManager`.

## 6. Make Melee Attacks Damage Boxes

Change `MeleeAttackStrategy` hit tracking from:

```cpp
std::vector<Enemy*> enemiesHit;
```

to:

```cpp
std::vector<GameObject*> objectsHit;
```

For each entity overlapping the melee hitbox:

1. Use `dynamic_cast<IDamageable*>`.
2. Skip entities that do not implement `IDamageable`.
3. Apply damage once if the `GameObject*` is not already in `objectsHit`.
4. Add the object to `objectsHit` after damage.
5. Keep existing positional knockback only when the object is an `Enemy`.
6. Do not move or knock back a box.
7. Create the existing impact effect at the hit object's center.

Clear `objectsHit` at the same combo boundaries where `enemiesHit` is currently
cleared. A box must not take damage twice from frames 1 and 2 of one swing.

Do not change EX-generation behavior as part of this feature.

## 7. Make Projectiles Damage Boxes

Adjust `GameManager::UpdateProjectiles()` collision order:

1. Update the projectile.
2. Check map tiles with
   `levelManager->IsSolidCollision(projectileBox, false)`.
3. Check intact boxes in `levelEntities`.
4. If a box is hit, call `TakeDamage(projectileDamage)`, create an impact
   effect, and destroy the projectile.
5. Continue with the existing active-Paladin or enemy collision logic only if
   the projectile has not already hit a wall or box.

Both player and enemy projectiles should damage boxes. This allows boxes to
serve as temporary cover. Enemy projectiles must still never damage other
enemies, and player projectiles must retain their current enemy-damage logic.

Damaging a box should not grant player EX because it is not an enemy hit.

## 8. Place Five Boxes in the Demo Map

Replace open `.` cells with `X` at these zero-based row/column positions in
`assets/levels/demo-big.txt`:

| Box | Row | Column | World center |
| --- | ---: | ---: | --- |
| 1 | 2 | 12 | `(400, 80)` |
| 2 | 9 | 20 | `(656, 304)` |
| 3 | 15 | 30 | `(976, 496)` |
| 4 | 24 | 15 | `(496, 784)` |
| 5 | 33 | 40 | `(1296, 1072)` |

All five selected cells are currently `.` floor cells and do not replace a
wall. Preserve every map row at its current width of 55 characters.

## State and Ownership Flow

```text
demo-big.txt contains X
    -> LevelManager parses X as floor plus entity marker
    -> EntityFactory creates DestructibleBox at tile center
    -> LevelManager owns it in levelEntities
    -> Movement/pathfinding/LOS treat its 32x32 body as solid
    -> Melee or projectile calls TakeDamage
    -> Box marks itself destroyed at zero health
    -> Box immediately stops drawing/blocking
    -> LevelManager deletes it at a safe update boundary
```

## Edge Cases

- Multiple damage calls after health reaches zero do nothing.
- A destroyed box is removed once and never deleted from inside combat loops.
- A projectile cannot damage a box and an enemy/player in the same update.
- Melee impact frames 1 and 2 cannot damage the same box twice in one swing.
- Boxes remain stationary and cannot be knocked into walls or other entities.
- Wave spawning cannot select a position overlapping an intact box.
- Ranged enemies cannot see or shoot through an intact box.
- Divers stop a lunge when their body reaches an intact box.
- Map reload deletes any surviving boxes through `ClearLevel()`.
- Destroying a box does not affect the active-enemy count or wave completion.

## Verification Plan

### Build verification

Run the repository command:

```text
cmake --build build --config Release
```

Confirm the new `DestructibleBox.cpp` source is discovered automatically.

### Demo-map checks

1. Enter the demo map and verify exactly five boxes appear.
2. Confirm every box aligns to one 32x32 tile.
3. Walk and dash into each side of a box; verify movement is blocked.
4. Confirm Chaser, Range, and Diver movement cannot pass through a box.
5. Confirm enemy pathfinding routes around occupied box tiles.
6. Confirm Range line of sight is blocked by a box.
7. Hit a box with each melee combo step and verify one damage event per swing.
8. Shoot a box with a player projectile and verify the projectile is consumed.
9. Put a box between an enemy projectile and the player; verify the box takes
   the hit and the player does not.
10. Destroy a box and verify it disappears, stops blocking, and makes its tile
    walkable again.
11. Clear or reload the level with boxes alive and verify there are no crashes
    or stale pointers.
12. Destroy every enemy while boxes remain and verify wave progression still
    occurs normally.

## Completion Criteria

- `DestructibleBox` derives from `GameObject` and occupies exactly 32x32 pixels.
- Five `X` markers create five boxes in the demo map.
- Boxes are damageable by melee, player projectiles, and enemy projectiles.
- One melee swing cannot hit the same box more than once.
- Boxes block player/enemy movement, pathfinding, line of sight, and spawn
  validation while intact.
- Destroyed boxes stop blocking immediately and are deleted safely later.
- Boxes are not treated as enemies and do not affect wave counts or EX gain.
- Level reload and shutdown clean up every remaining box.
- The project compiles through all modified source files.
