# Implementation Plan: Minimap UI & Renderer Fix

## 1. Bug Fixes (Completely Dark Map & Freeze)
**Root Cause:**
When `BakeLevel()` was implemented, it created a 140x140 grid where empty void space is flagged as `tileType = 2`. In `TilemapRenderer::DrawRoom`, iterating through all 19,600 tiles and calling `DrawRectangleRec(BLACK)` for every single void tile causes massive frame drops (rendering the character unable to move). Furthermore, because the camera spawns at `(0,0)` and the player spawns at `(3240, 3240)`, the slow camera pan over 19,600 black void rectangles makes the screen appear completely dark and frozen.

**Fix:**
- We will modify `TilemapRenderer::DrawRoom` to completely skip drawing `tileType == 2` (`if (tileType == 2) continue;`). This will instantly restore 60 FPS.
- We will ensure `LevelManager::UpdateLevel` correctly sets `node->isDiscovered = true` whenever the player's bounding box intersects a room's trigger bounds.

## 2. Minimap UI Architecture
We will create a new dedicated UI component for the minimap:
- **Files:** `include/UI/MinimapRenderer.h` and `src/UI/MinimapRenderer.cpp`.
- **Function:** `void MinimapRenderer::Draw(const LevelMap& levelMap, int currentGridX, int currentGridY);`

## 3. Minimap Visual Representation & Fog of War
We will anchor the minimap to the top-right corner of the HUD.
- **Grid Iteration:** We will iterate through `levelMap.grid`. We will only draw a `RoomNode` if `node->isDiscovered == true`.
- **Room Visualization:** Each room will be represented as a `12x12` pixel square.
  - **Current Room:** Pulsing or solid `WHITE` square.
  - **Cleared Room:** `DARKGRAY`.
  - **Discovered but Uncleared:** `LIGHTGRAY`.
- **Special Icons:**
  - `RoomType::SPAWN`: Overlaid with a smaller `GREEN` square.
  - `RoomType::BOSS`: Overlaid with a smaller `RED` square.
  - `RoomType::CHEST`: Overlaid with a smaller `GOLD` square.
- **Corridors:** If two adjacent rooms are linked, and at least one is discovered, we will use `DrawLineEx` to draw a small connecting bridge between their minimap squares.

## 4. Game Integration
- We will update `GameManager::DrawHUD()` to invoke `MinimapRenderer::Draw()`, passing the `levelMap` and the player's currently active room coordinates.
- We will update the CMake build system to include `src/UI/MinimapRenderer.cpp`.

> [!TIP]
> **Dynamic Centering:** To ensure the minimap doesn't block too much screen space if the dungeon grows massive in the future, I will implement dynamic centering. The minimap will locally center around the `currentGridX/Y` coordinate, mapping the local cluster to the fixed top-right HUD region, rather than drawing absolute grid offsets.
