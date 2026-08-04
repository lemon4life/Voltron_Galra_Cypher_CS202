# Implementation Plan: Procedural Dungeon Builder

We will implement a Random Walker algorithm to generate a multi-room dungeon layout, connect them with structured corridors, and bake the entire graph into a unified, playable world map.

## 1. Fixed Geometry Configuration
We will add the following definitions to `Constants.h` to standardize generation:
- `ROOM_TILE_SIZE = 15`
- `CORRIDOR_LENGTH = 5`
- `CORRIDOR_WIDTH = 3`
- `TILE_SIZE = 32.0f` *(Note: Sticking to 32 instead of 16 to ensure 100% compatibility with your legacy maps and physics!)*

## 2. Macro Generation (Dungeon Builder)
We will rewrite `LevelMap::Generate()` to use a random walker approach:
- **Grid Initialization:** Create a 7x7 grid.
- **Spawn Room:** Start at `[3][3]`, mark it as `SPAWN`.
- **Random Walker:** Iteratively step into adjacent, empty grid cells and mark them as `BATTLE`. We will repeat this until a target of `MIN_ROOMS` (e.g., 6) is reached.
- **Boss Room:** The final room placed will be marked as `BOSS`.
- **Door Flags:** As the walker steps, we will set boolean links (e.g., `node->hasNorthDoor = true`) between the connected rooms.

## 3. World Space Translation & Baking
We will implement `LevelMap::BakeLevel()`, which creates a single, giant `RoomTemplate` containing the entire dungeon.
- **Coordinate Math:** The distance between grid cells is `(ROOM_TILE_SIZE + CORRIDOR_LENGTH)`. 
  - `worldGridX = node->gridX * (ROOM_TILE_SIZE + CORRIDOR_LENGTH)`
  - `worldGridY = node->gridY * (ROOM_TILE_SIZE + CORRIDOR_LENGTH)`
- **Room Carving:** For each generated `RoomNode`, we carve out a `15x15` square of floors (`0`) surrounded by walls (`1`).
- **Corridor Carving:** If a room has a door flag (e.g., `hasNorthDoor`), we carve a corridor up to the next room, overriding walls with floors.
- **Door Anchors:** We will inject door markers (`20`) into Layer 1 exactly where the corridors meet the rooms. This ensures the `LevelManager`'s lockdown system still works perfectly.
- **Trigger Bounds:** We will calculate and assign the absolute world pixel `triggerBounds` for each `RoomNode` during baking.

## 4. Integration
- In `LevelManager::GenerateDungeon()`, we will call `levelMap.Generate()`, then `levelMap.BakeLevel()`. The returned mega-template becomes the `activeRoom`.
- `LevelManager` will use the mega-template to generate `currentRoomWalls`.
- The camera will freely track the player, except when a room locks (the clamp logic implemented previously will still work perfectly since it targets the specific `activeRoomNode->triggerBounds`!).

## Open Questions for You
> [!IMPORTANT]  
> The prompt mentioned `TILE_SIZE = 16`. The existing collision engine, character speeds, and legacy maps are heavily tuned for `32`. Changing this to 16 globally might break existing physics bounds and legacy maps. Is it acceptable that I enforce `TILE_SIZE = 32.0f` to guarantee stability?
