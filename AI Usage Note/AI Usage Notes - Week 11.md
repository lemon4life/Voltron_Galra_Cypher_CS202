
## Prompt 117c — Camera Sub-pixel Rendering Fix (2026-08-11)

**Prompts used:**
- "the Map is still bleeding."

**Purpose:** Fix persistent bleeding/grid lines appearing between tiles during camera movement.

**Root Causes Identified:**
- Even with inset texture coordinates, Raylib's `BeginMode2D` applies floating point camera `target` and `offset` values. When rendering tiles that are snapped to integers in world space (via `std::floor`), the floating point camera causes the rendering pipeline to draw them at sub-pixel screen coordinates. Due to precision errors in `TEXTURE_FILTER_POINT`, 1-pixel gaps can appear between adjacent tiles, exposing the background or debug layers underneath.

**Content Generated / Changes Made:**
- `include/Core/Manager/CameraManager.h`: Added `GetRenderCamera()` which returns a copy of the camera with `target` and `offset` strictly rounded down via `std::floor`.
- `src/main.cpp`: Replaced `BeginMode2D(camera)` with `BeginMode2D(CameraManager::GetInstance().GetRenderCamera())` to ensure all world rendering is perfectly pixel-aligned.
- `src/Core/Level/RoomEditorState.cpp`: Applied the same `std::floor` logic to the local editor camera right before calling `BeginMode2D`, ensuring the level editor doesn't suffer from sub-pixel gaps either.

## Prompt 117d — DoorGate Animation Texture Bleeding Fix (2026-08-11)

**Prompts used:**
- "I realized the bleeding (3 vertical purple line start from the transfer gate room, check it out what is happening?"

**Purpose:** Fix exactly 3 vertical purple lines appearing at the entrance of the transfer gate room.

**Root Causes Identified:**
- The 5-tile wide `DoorGate` consists of 5 separate entities side-by-side. 
- Because their sprite sheets (`Galra_Door_8.png` and `Transfer_gate.png`) contain multiple animation frames packed tightly side-by-side, rendering them without insetting the `src` rectangle caused Raylib's `TEXTURE_FILTER_POINT` scaling to bleed into adjacent animation frames in the spritesheet.
- The 5 adjacent doors create 4 seams. The outermost seams overlap the physical wall boundary, leaving exactly 3 seams visible on the walkable corridor floor, explaining the "3 vertical purple lines".

**Content Generated / Changes Made:**
- `src/Entities/Props/DoorGate.cpp`: Inset the `src` rectangle by `0.1f` on the left and right bounds (`frameWidth - 0.2f`) for all animation states to prevent horizontal texture bleeding from adjacent frames.
- `src/Core/Manager/LevelManager.cpp`: Applied the exact same `0.1f` horizontal inset logic to the `gateTexture` used for rendering the EXIT transfer gate.

## Prompt:
the mini map tracking room doesn't track room correctly, it highlight other instead of the current one

## Purpose:
Fix a bug where the minimap tracked the player's position incorrectly and highlighted the wrong rooms on the minimap grid.

## Content Generated:
- `main.cpp`: Fixed the `currentGridX` and `currentGridY` minimap grid calculations to divide the player's physical coordinates by `Constants::RENDER_TILE_SIZE` instead of `TILE_SIZE * GLOBAL_SCALE`. Previously, the code mistakenly divided by `32.0f` instead of `16.0f`, causing the calculated grid coordinates to be exactly half of the correct values, resulting in the minimap tracking the wrong room entirely.

## Prompt:
Context:
Our custom rooms generated from the editor are highly dense with objects, making random "guess-and-check" enemy spawning inefficient and prone to infinite loops. 

Task:
Implement a "Free Space Pool" algorithm inside the `Room` or `DungeonGenerator` class to efficiently cache all guaranteed-safe spawn locations when a room is loaded, preventing enemies from spawning on top of objects or each other.

## Purpose:
Implement an O(1) Free Space Pool algorithm for safe enemy spawning in procedural dungeons to prevent infinite loops in highly dense rooms.

## Content Generated:
- `RoomNode.h`: Added `std::vector<Vector2> availableSpawnNodes` and `void CalculateWalkableGrid(class LevelManager* lm)` to the `RoomNode` class.
- `LevelManager.cpp`: Implemented `RoomNode::CalculateWalkableGrid` to scan the room grid at 0.5 tile resolution and push free coordinates into `availableSpawnNodes` using `IsSolidCollision`. Called this method for each node at the end of `GenerateDungeon`. Refactored `GetSafeSpawnPosition` to select a random index from `availableSpawnNodes`, returning the position in O(1) time, and then swapping the selected element with the last element and popping it to guarantee enemies do not spawn on top of each other.
## Hotfix: Boss Spawning

- Fixed an issue where the Boss was skipped if the `availableSpawnNodes` array was fully exhausted or empty by ensuring the fallback logic in `LevelManager::GetSafeSpawnPosition` returns `true` instead of `false`.
