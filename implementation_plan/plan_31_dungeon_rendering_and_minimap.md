# Implementation Plan: Dungeon Rendering, Scaling, and Minimap Upgrades

This plan summarizes the sequence of fixes and features applied to resolve compilation issues, implement dynamic room sizing, refine the top-down rendering mechanics, and upgrade the player navigation UI (minimap and camera).

## 1. Bug Fixes (Compilation & Paths)
**Root Cause:**
Previous architectural changes left residual undeclared enumerations (`RoomType::ENEMY` instead of `RoomType::BATTLE`) and obsolete constants (`Constants::ROOM_TILE_SIZE`) causing CMake build failures. Additionally, centering smaller 20x20 rooms inside larger 32x32 cells severed corridor paths, as the corridor algorithm previously only carved outwards towards the East and South assuming rooms fully populated the cells.
**Fix:**
- Corrected all instances of `RoomType::ENEMY` to `RoomType::BATTLE`.
- Corrected `Constants::ROOM_TILE_SIZE` to `Constants::MAX_ROOM_TILE_SIZE` in `main.cpp`.
- Added explicit `node->west` and `node->north` directional corridor carving in `Tilemap.cpp::BakeLevel` so that a room reliably bridges the local internal cell offset (e.g., from `x=6` backward to `x=0`), seamlessly attaching to adjacent cells.

## 2. Dynamic Room Sizing & Wide Paths
**Goal:** Increase the space available for enemy combat encounters without blowing up the sizes of spawn or chest rooms, and widen the interconnecting corridors for smoother player traversal.
**Implementation:**
- Introduced `Constants::MAX_ROOM_TILE_SIZE` (32) and `Constants::NORMAL_ROOM_TILE_SIZE` (20).
- Configured `RoomType::BATTLE` and `RoomType::BOSS` to occupy the full 32x32 cell. 
- Normal interaction rooms (Spawn, Exit, Chest) are hardcapped at 20x20 but are offset to perfectly center within their 32x32 bounding cell to maintain absolute grid alignment.
- Corridors were widened from 3 tiles to 5 tiles (`Constants::CORRIDOR_WIDTH`).

## 3. Top-Down Depth Rendering & Void Fixes
**Goal:** Prevent bottom-facing map walls from drawing as flat tiles, properly frame the procedural dungeon map in a dark void, and scale up interactive assets (Exit Gate).
**Implementation:**
- Updated the logic in `TilemapRenderer::DrawRoom` to check if a tile directly below a wall is a `Floor` OR a `Void (tileType == 2)`. If true, the bottom walls properly render a front-facing texture, maintaining the 2.5D visual depth even on the edge of the map.
- Replaced `ClearBackground(DARKGRAY)` with `ClearBackground(BLACK)` in `main.cpp` so the out-of-bounds void doesn't have an ugly gray backdrop.
- Scaled up the exit gate from 2x tile size to 4x tile size, and implemented frame-time based animation cycling (`GetTime() * 10 % 8`) for the gate texture.

## 4. Minimap Overhaul & Radar Functionality
**Goal:** Provide the player with a roguelike "radar" to anticipate adjacent room types before entering, and display correct corridor connections.
**Implementation:**
- Refactored `MinimapRenderer::Draw` to introduce an `isRevealed` helper. Rooms now render if they are discovered OR if they share an edge with a discovered room.
- Rendered undiscovered-but-revealed rooms with a darker gray shade (`Fade(DARKGRAY, 0.9f)`).
- Added an exclamation mark (`!`) icon graphic for `RoomType::BATTLE` rooms to warn players of danger.
- The minimap now draws connecting corridor rectangles exclusively between revealed rooms.

## 5. Gameplay Kinematics (Camera & Room Lockdown)
**Goal:** Widen the field of view to better suit the 32x32 arenas, and ensure the player cannot bypass battle barricades in the newly widened 5-tile corridors.
**Implementation:**
- Adjusted the `CameraManager::UpdateCamera` zoom lerp target by applying a `0.75f` multiplier, successfully pulling the camera back by 25%.
- Fixed a major room-escape bug by modifying `Tilemap.cpp::BakeLevel`'s door anchor (`layer1_objects = 20`) placement. It now iterates across the full `Constants::CORRIDOR_WIDTH`, spawning a 5-tile wide solid barricade to lock the player inside combat rooms.
