# Adjustments to EnemyPathManager — Partial Wall Handling

## Goal

Fix gaps in `EnemyPathManager` where A\* pathfinding does not fully respect
partial-wall tiles (tile IDs 1-3 and 12-14). These tiles only occupy 9 px of a
32 px tile, and the current pathfinding checks at tile centers can miss
collisions that occur along tile edges during diagonal movement and
inter-waypoint interpolation.

## Background

### How Pathfinding Uses LevelManager

`EnemyPathManager` calls four `LevelManager` methods:

| Method | Purpose |
| --- | --- |
| `IsSolidCollision(Rectangle)` | Core collision — tests an enemy bounding box against both tile layers |
| `WorldToTile(Vector2)` | Converts enemy/world positions to tile-grid coordinates |
| `TileToWorld(int, int)` | Converts tile coordinates to world-space tile-center positions |
| `GetEntities()` | Iterates all entities for local avoidance separation |

The A\* call chain is:

```
FindPath()
  -> CanMoveBetweenTiles()
       -> IsBodyClearAtTile()
            -> TileToWorld()         // tile center
            -> IsBodyClearAtWorldPosition()
                 -> GetBodyAtWorldPosition()  // places enemy bbox at tile center
                 -> IsSolidCollision()        // rectangle vs tile grid
```

### Partial Wall Geometry

Defined in `LevelManager::IsSolidCollision()`:

- **Tiles 1-3**: Right 9 px solid — `solidPart = { c*32+23, r*32, 9, 32 }`
- **Tiles 12-14**: Left 9 px solid — `solidPart = { c*32, r*32, 9, 32 }`

`IsSolidCollision()` correctly does per-tile rectangle intersection with the
enemy's real bounding box. It is **not** a binary whole-tile check.

## Identified Issues

### Issue 1 — Diagonal corner-cutting through partial walls

**Location**: `CanMoveBetweenTiles()` in `EnemyPathManager.cpp:82-104`

When A\* evaluates a diagonal move, it checks the horizontal and vertical
intermediate tiles at their **tile centers**:

```cpp
Tile horizontalTile = { current.x + dx, current.y };
Tile verticalTile = { current.x, current.y + dy };
```

A diagonal path does not pass through these centers — it cuts across the corner.
A partial wall in an intermediate tile may not overlap the enemy bbox placed at
that tile's center, but the diagonal swept path could clip through the solid
strip.

**Example**:

```
Current tile (empty)       -> Horizontal neighbor (tile ID 1, right-partial wall)
                            ↘ Diagonal neighbor (empty)
```

Enemy body at horizontal-neighbor center may not reach the right 9 px, but the
actual diagonal transition clips through it.

**Severity**: Medium. Depends on whether partial walls are placed at diagonal
corners in level layouts.

### Issue 2 — Inter-waypoint movement is unchecked

**Location**: A\* returns tile-center waypoints via `TileToWorld()`; the enemy
interpolates between them over multiple frames.

A\* only verifies collision at tile centers. Between two consecutive tile-center
waypoints, the enemy passes through tile edges and corners. No collision check
is performed along this swept path. An enemy could clip through a partial wall's
solid strip while transitioning between two tiles that are individually clear at
their centers.

The `GetLocalAvoidanceDirection()` wall-sliding probe mitigates but does not
fully prevent this, because it probes only 18 px ahead in the desired direction
and only tries perpendicular fallbacks — it cannot guarantee the full swept path
is clear.

**Severity**: Medium-High. This applies to every frame of enemy movement between
waypoints, not just diagonal transitions.

### Issue 3 — IsWalkableTile inconsistency (not a pathfinding bug)

**Location**: `LevelManager::IsWalkableTile()` at `LevelManager.cpp:344-360`

`IsWalkableTile()` uses `IsBlockingTileID()` which treats tiles 1-14 as fully
blocking. `EnemyPathManager` does **not** use `IsWalkableTile()` — it calls
`IsSolidCollision()` directly — so pathfinding is unaffected. However, any other
system that calls `IsWalkableTile()` (e.g., spawn validation, player collision)
gets an overly conservative result for partial-wall tiles.

**Severity**: Low for pathfinding; potential concern for other systems.

## Proposed Fixes

### Fix 1 — Add edge sampling to diagonal checks in CanMoveBetweenTiles

**File**: `src/Core/Manager/EnemyPathManager.cpp` — `CanMoveBetweenTiles()`

Instead of checking the intermediate tiles only at their centers, also sample
collision at the two corner points where the diagonal path crosses tile edges.
For a diagonal move from `(cx, cy)` to `(cx+dx, cy+dy)`, test:

1. Center of horizontal intermediate tile (existing).
2. Center of vertical intermediate tile (existing).
3. The corner point `(cx+dx, cy)` in world space — i.e., the tile-edge crossing
   point of the horizontal intermediate.
4. The corner point `(cx, cy+dy)` in world space — i.e., the tile-edge crossing
   point of the vertical intermediate.

For points 3 and 4, create a small bounding box centered on the crossing point
(same dimensions as the enemy body) and run `IsSolidCollision()` on it. This
catches partial walls that sit at tile edges without extending to tile centers.

**Pseudocode**:

```cpp
bool CanMoveBetweenTiles(LevelManager* lm, Enemy* e, Tile cur, Tile nb) {
    if (!IsBodyClearAtTile(lm, e, nb)) return false;

    int dx = nb.x - cur.x;
    int dy = nb.y - cur.y;
    if (dx == 0 || dy == 0) return true;  // cardinal move — no corner-cut

    // Existing center checks for intermediate tiles
    if (!IsBodyClearAtTile(lm, e, {cur.x+dx, cur.y}))   return false;
    if (!IsBodyClearAtTile(lm, e, {cur.x, cur.y+dy}))   return false;

    // New: edge-crossing point checks
    Rectangle body = e->GetBoundingBox();
    Vector2 pos = e->GetPosition();
    float offsetX = pos.x - body.x;
    float offsetY = pos.y - body.y;

    float edgeX1 = (float)(cur.x + dx) * 32.0f + 16.0f; // horizontal-intermediate center X
    float edgeY  = (float)cur.y * 32.0f + 16.0f;         // current tile center Y
    Rectangle box1 = { edgeX1 - offsetX, edgeY - offsetY, body.width, body.height };
    if (lm->IsSolidCollision(box1)) return false;

    float edgeX2 = (float)cur.x * 32.0f + 16.0f;         // current tile center X
    float edgeY2 = (float)(cur.y + dy) * 32.0f + 16.0f;  // vertical-intermediate center Y
    Rectangle box2 = { edgeX2 - offsetX, edgeY2 - offsetY, body.width, body.height };
    if (lm->IsSolidCollision(box2)) return false;

    return true;
}
```

This adds two `IsSolidCollision()` calls per diagonal candidate. The cost is
minimal because `IsSolidCollision()` only iterates the 1-4 tiles overlapping the
small bounding box.

### Fix 2 — Add swept-path clearance in GetNextMoveTarget

**File**: `src/Core/Manager/EnemyPathManager.cpp` — `GetNextMoveTarget()`

When validating the current first waypoint, also check whether the straight line
from the enemy's current position to the waypoint intersects any solid tile.
This prevents the enemy from receiving a target that requires clipping through a
partial wall between two clear tile centers.

**Approach**: Use a lightweight swept check by sampling collision at the
midpoint between the enemy and the waypoint, in addition to the existing
endpoint check.

```cpp
// Inside GetNextMoveTarget(), after confirming the target is clear:
Vector2 enemyPos = enemy->GetPosition();
Vector2 mid = { (enemyPos.x + targetPosition.x) * 0.5f,
                (enemyPos.y + targetPosition.y) * 0.5f };
if (!IsBodyClearAtWorldPosition(levelManager, enemy, mid)) {
    pathAgent->ClearTargetPosition();
    return fallbackTarget;
}
```

For longer moves (spanning multiple tiles), also sample at the 1/4 and 3/4
points. This can be gated on distance:

```cpp
float dist = Vector2Length(Vector2Subtract(targetPosition, enemyPos));
int samples = (dist > 32.0f) ? 3 : 1; // quarter, half, three-quarter if > 1 tile
for (int i = 1; i <= samples; ++i) {
    float t = (float)i / (float)(samples + 1);
    Vector2 sample = { enemyPos.x + (targetPosition.x - enemyPos.x) * t,
                       enemyPos.y + (targetPosition.y - enemyPos.y) * t };
    if (!IsBodyClearAtWorldPosition(levelManager, enemy, sample)) {
        pathAgent->ClearTargetPosition();
        return fallbackTarget;
    }
}
```

**Severity reduction**: This turns Issue 2 from Medium-High into Low. Full swept
path verification would be expensive; midpoint sampling catches the most common
partial-wall clipping cases (enemies cutting through a single-tile-thick wall).

### Fix 3 — Align IsWalkableTile with partial-wall semantics

**File**: `src/Core/Manager/LevelManager.cpp` — `IsWalkableTile()`

Option A (minimal): Keep `IsWalkableTile()` conservative for full-tile blocking
and document that it treats partial walls as blocking. Any system that needs
partial-wall-aware checks should use `IsSolidCollision()` directly.

Option B (precise): Change `IsWalkableTile()` to accept a `Rectangle` (or
optional bounding box) and delegate to `IsSolidCollision()`:

```cpp
bool LevelManager::IsWalkableTile(int x, int y, Rectangle* testBox) const {
    if (x < 0 || y < 0 || y >= gridRows || x >= gridCols) return false;
    if (testBox) return !IsSolidCollision(*testBox);
    // Fallback: conservative full-tile check
    return !IsBlockingTileID(tileID1) && !IsBlockingTileID(tileID2);
}
```

This is low priority — only needed if non-pathfinding systems need
partial-wall-aware tile queries.

## Proposed Files

### Modified files

- `src/Core/Manager/EnemyPathManager.cpp` — Fix 1 and Fix 2
- `src/Core/Manager/LevelManager.cpp` — Fix 3 (optional)
- `include/Core/Manager/LevelManager.h` — Fix 3 signature change (optional)

No new files are needed. CMake already discovers all `src/*.cpp` files.

## Verification Plan

### Static verification

- Confirm `CanMoveBetweenTiles()` now tests at least 4 points per diagonal
  move (2 existing center checks + 2 new edge-crossing checks).
- Confirm `GetNextMoveTarget()` samples the midpoint (and optionally
  quarter-points) between enemy and waypoint before accepting the target.
- Confirm no new includes or dependencies are introduced.

### Build verification

Run:

```text
cmake -S . -B build && cmake --build build --config Release
```

Stop if the build fails; do not force an alternate build.

### Gameplay checks

1. Place partial-wall tiles (IDs 1-3, 12-14) at diagonal corners in a test
   level. Confirm enemies do not path through them.
2. Create a corridor with partial walls on both sides. Confirm enemies navigate
   without clipping.
3. Place a single-tile-thick partial wall between two open areas. Confirm
   enemies path around it rather than through the wall's clear-center shortcut.
4. Verify that normal (fully solid) wall pathfinding is unchanged.
5. Confirm enemy avoidance steering still slides along partial walls correctly.
6. Confirm Range and Diver enemies still reach valid firing positions near
   partial walls without getting stuck.

## Completion Criteria

- A\* `CanMoveBetweenTiles()` checks collision at diagonal edge-crossing
  points, not just intermediate tile centers.
- `GetNextMoveTarget()` validates that the path from the enemy to its next
  waypoint does not pass through a partial wall.
- `IsWalkableTile()` is either aligned with partial-wall semantics or
  documented as conservative.
- Enemies respect partial walls during diagonal pathfinding transitions.
- Enemies do not clip through partial walls between waypoints.
- The project builds cleanly.

## Objective Design Assessment

### Strengths of the proposed fixes

- **Low cost**: Fix 1 adds only two `IsSolidCollision()` calls per diagonal
  neighbor evaluation. Fix 2 adds 1-3 calls per target validation. Both are
  negligible compared with the 500-step A\* limit and per-frame entity updates.
- **Minimal code change**: Both fixes modify existing functions without
  restructuring the pathfinding pipeline.
- **Backward compatible**: Fully solid walls and partial-wall tiles that are
  wide enough to block the enemy at tile center continue to work as before.

### Weaknesses

- Midpoint sampling (Fix 2) is an approximation. A wall thinner than the
  sampling interval could theoretically be missed, but partial walls in this
  project are 9 px thick and enemy bodies are ~24 px wide, making false
  negatives unlikely.
- Fix 1 only covers 90-degree diagonal corners. If enemies move at other angles
  (e.g., from local avoidance steering), the edge-crossing sampling may not
  apply. This is acceptable because A\* only produces grid-aligned diagonals.

### Verdict

The current pathfinding is correct for tiles that are fully solid or fully open,
and correctly uses `IsSolidCollision()` for per-tile rectangle checks. The gaps
are in the transition space between tile centers. The proposed fixes close these
gaps with minimal performance overhead and no structural changes to the
pathfinding architecture.
