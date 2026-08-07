# Remove Legacy Map & Introduce Template-Based Rooms

This plan outlines the removal of the old mission map and dialogue options, as well as a proposed architecture for designing pre-made rooms that can be injected into the procedural dungeon generator.

## Proposed Changes

### 1. Dialogue & UI Updates
- **[MODIFY]** [`assets/story/intro.txt`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron Mission Galra Cypher/assets/story/intro.txt): Remove the "I will go check the Hall (Start mission in the old way)" option. Update the remaining option to simply start the mission.

### 2. Game State Management
- **[MODIFY]** [`src/main.cpp`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron Mission Galra Cypher/src/main.cpp):
  - Remove the legacy `ResetGame` function (which loaded the old mission).
  - Rename `ResetGameModular` to `ResetGame` (which initializes the procedural dungeon).
  - Update the dialogue parsing logic to only listen for the start mission ID (removing the `-1` and `-2` branching).

### 3. Clean up Legacy Map Files
- **[DELETE]** `assets/map/level1_*.csv` files
*(Note: `demo-big_*.csv` files, `hub_Tile Layer 1.csv`, and the `LevelManager::LoadLevel()` function will be kept per your request).*

---

## Room Design Strategy: Pre-Authored Templates

To design rooms with pre-placed objects, we will move to a **Template-Based Room Generation** system. You will create `.csv` files using Tiled (or manually), and the procedural dungeon generator will randomly pick a template for each room it creates.

Here is exactly how the CSV files will be structured and what numbers you need to use:

### CSV Layer Structure
Each template will consist of three CSV files (e.g., if your room is 20x20 tiles):
1. **`room_name_Tile Layer 1.csv`** (Floors & Void)
   - `-1`: Empty/Void (no floor)
   - `>0` (Any positive number): Floor tile. The generator will automatically pick random floor textures for these.
2. **`room_name_Tile Layer 2.csv`** (Walls)
   - `-1`: Empty (no wall)
   - `>0` (Any positive number): Wall tile. The generator will automatically assemble the wall meshes here.
3. **`room_name_Game Objects.csv`** (Entities & Props)
   - `-1`: Empty (no object)
   - `1`: Destructible Box
   - `2`: Chaser Enemy
   - `3`: Range Enemy
   - `4`: Diver Enemy
   - `10`: Prop 1 (Tall Object)
   - `11`: Prop 2 (Small Object)
   - `12`: Mock Wall (Decorative Wall)

### How It Works in Code
We will update `LevelManager` to load all of these small room CSVs at startup and store them in a list of `RoomTemplate` objects.

When `LevelManager::GenerateDungeon()` places a new room node in the BSP tree, it will:
1. Pick a random `RoomTemplate` from the pre-loaded pool that matches the room type.
2. **Translate the CSVs:** It will read `Tile Layer 1` to create the floor bounds, `Tile Layer 2` to create the walls, and `Game Objects` to spawn the entities exactly where you put them.
3. **Dynamic Doors**: The generator will still dynamically punch holes in the outer walls (`Tile Layer 2`) of the template to create doors and corridors that link to adjacent rooms.

This way, you can design exact combat arenas with precise enemy/prop locations in Tiled, but they will be randomly connected and arranged in a different layout every time you play!

---

## User Review Required
> [!IMPORTANT]
> Does the CSV numbering system and layer structure look good to you? Once you approve, I will proceed with removing the legacy map files and dialogues so you have a clean slate to begin crafting your room templates.
