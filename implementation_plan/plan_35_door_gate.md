# Animated 2.5D DoorGate Implementation

This plan outlines the architecture for introducing an animated `DoorGate` entity to seamlessly transition between open floor and locked walls during room battles.

## Proposed Changes

### 1. New DoorGate Entity
- **[NEW]** `src/Entities/Props/DoorGate.cpp` & `include/Entities/Props/DoorGate.h`
  - A new class inheriting from `GameObject`.
  - Implements the State Machine: `OPEN`, `CLOSING`, `LOCKED`, and `OPENING`.
  - Handles the 8-frame animation playback.
  - **Collision**: Dynamically toggles its AABB box. It will be solid when `CLOSING`, `LOCKED`, or `OPENING`. It will have zero/inactive collision when `OPEN`.
  - **Rendering & Offsets**: Translates the 16x32 source sprite to perfectly overlay the 32x32 world-scaled grid tile.
  - **Two-Pass Sorting**: 
    - `DrawBaseLayer()`: Always renders the bottom 16x16 (scaled to 32x32) beneath the player.
    - `AddDepthRenderItems()`: Renders the top 16x16 above the player (if player Y > door base) **only** when the state is not `OPEN`.

### 2. Core Structure Updates
- **[MODIFY]** `include/Entities/GameObject.h`: 
  - Add `DoorGate` to `GameObjectType` enum so `LevelManager` can quickly identify it during rendering loops.
- **[MODIFY]** `include/Core/Level/Tilemap.h`:
  - Add a `std::vector<DoorGate*> doors` collection directly to `RoomNode`. This allows instantaneous state toggling for all doors tied to a specific room.

### 3. Level Manager Integration
- **[MODIFY]** `src/Core/Manager/LevelManager.cpp`:
  - **Generation**: Inside `GenerateDungeon()`, scan the room grids for the door anchors (the `20` ID). Instantiate `DoorGate` objects, placing them into `levelEntities` and binding them to the parent `RoomNode`.
  - **Battle Lockdown**: In `LevelManager::Update`, when `activeRoom` transitions to `ACTIVE`, loop through its `doors` array and call `SetState(CLOSING)`.
  - **Battle Cleared**: When the room clears, loop through the `doors` array and call `SetState(OPENING)`.
  - **Cleanup**: Remove the legacy logic that maintained and checked the raw `doorColliders` rectangles. `DoorGate` will naturally handle its own collision interception via the standard entity loop.

## Asset Verification
- The sprite `assets/tileset/Galra_Door_8.png` has been located and will be used as the texture sheet.

## User Review Required
> [!IMPORTANT]
> Please review the proposed architecture. This cleanly bridges the gap between your procedural generation, battle states, and our new dynamic 2.5D sorting pipeline. If you approve, I will proceed with creating the class and hooking it up!
