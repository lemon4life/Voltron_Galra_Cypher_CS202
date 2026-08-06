# 2.5D Depth Sorting and Procedural Props

## Goal
Implement a 2.5D depth-sorting rendering pipeline for tall entities (16x32) and walls. Split their rendering into a base layer (bottom 16x16) and a depth-sorted overhang layer (top 16x16). Implement procedural spawning for new props (`box`, `object_1`, `object_2`) in BATTLE rooms.

## Open Questions
- Does `object_1_8.png` have an animation speed requirement, or should it run at a standard 8-10 FPS?
- Should indestructible props (object_1, object_2) block enemy pathfinding? (Assuming yes, since they have solid AABB collisions).
- Are the new prop textures already loaded in `AssetManager`? (Assuming I need to add them to `AssetManager`).

## Proposed Changes

### Core Rendering Architecture (`src/main.cpp`, `src/Core/Manager/LevelManager.cpp`)
To satisfy the 2-pass requirement with Y-sorting:
- Introduce a unified `DrawDepthSorted()` system in `main.cpp` or `LevelManager` that handles the second pass.
- **Pass 1 (Base Layer):** 
  - `LevelManager::DrawLevelBase()`: Draws floors and the bottom 16x16 of walls and static props.
- **Pass 2 (Depth Layer):**
  - We need a struct `DepthRenderItem` containing drawing logic (or a texture, source rect, dest rect) and a `ySort` value.
  - `LevelManager` generates a list of `DepthRenderItem` for the top 16x16 of all walls and props.
  - Characters (Players, Enemies) are added to this list with their `ySort` = their feet Y-coordinate.
  - Sort the list by `ySort` ascending.
  - Draw all items in the list.

#### [MODIFY] `include/Core/Manager/LevelManager.h` & `src/Core/Manager/LevelManager.cpp`
- Split `DrawLevel()` into `DrawLevelBase()` and `GetDepthRenderItems()`.
- `GetDepthRenderItems()` will iterate over the tilemap/props and return the top-half render instructions.
- Add lists and generation logic for procedural props.

#### [MODIFY] `src/main.cpp`
- Update the main rendering loop to call `levelManager.DrawLevelBase()`, gather depth items from `levelManager`, `teamManager`, and `waveManager`, sort them, and execute their draw calls.

### Prop Entities and Assets
- Load `box.png`, `object_1_8.png`, and `object_2.png` in `AssetManager`.

#### [NEW] `include/Entities/Props/Prop.h` & `src/Entities/Props/Prop.cpp`
- Create a `Prop` class inheriting from `GameObject`.
- Support three types: Destructible Box, Animated Indestructible, Static Indestructible.
- Override `Draw()` to do nothing (since rendering is handled by the unified depth pass), or provide a method to get its Pass 1 and Pass 2 rendering structs.
- Collision: AABB only covers the bottom 16x16 (scaled to world size).

### Procedural Dungeon Updates (`src/Core/Level/Tilemap.cpp`)
- Update `LevelMap::BakeLevel()` or `RoomTemplate` to support prop placement.
- When baking a BATTLE room, randomly pick 2 to 6 empty coordinates (aligned to the 16x16 grid).
- Ensure they don't spawn within 2 tiles of the doors (to avoid blocking).
- Populate a new `layer2_props` grid in `RoomTemplate`.

### Physics & Collision Updates
- Ensure `IsSolidCollision()` in `LevelManager` checks the new `Prop` AABBs.
- The player will correctly collide with the bottom 16x16, allowing them to walk "behind" the top 16x16.

## Verification Plan
1. **Automated Tests**: Clean build via CMake.
2. **Manual Verification**: 
   - Walk behind and in front of walls and props to verify the 2.5D sorting overlap.
   - Verify collision only occurs on the bottom half of props/walls.
   - Verify procedural generation places props correctly without blocking paths.
   - Verify animated prop (`object_1_8`) plays its animation correctly despite being split.
