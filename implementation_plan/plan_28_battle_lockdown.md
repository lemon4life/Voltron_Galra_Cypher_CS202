# Implementation Plan: Battle Room Lockdown System

We will implement the Room Lockdown system which dynamically traps the player inside uncleared battle rooms until the Wave Manager completes its spawn cycles.

## 1. RoomNode State & Triggers
- **RoomState Enum:** We will introduce `enum class RoomState { IDLE, LOCKED, CLEARED };` in `RoomNode.h`.
- **Trigger Bounds:** `RoomNode` will have a defined `Rectangle triggerBounds`. For our current 1x1 test room, we will define this as the inner area of the room (inset by 2 tiles from the walls).
- **Player Detection:** `LevelManager::UpdateLevel()` will check the active paladin's position. If the player steps inside the `triggerBounds` of an `IDLE` battle room, it will change the state to `LOCKED` and trigger the lockdown sequence.

## 2. Lockdown Behavior & Door Barriers
- **Dynamic Door Generation:** When the room enters the `LOCKED` state, we will dynamically inject door barriers into the `currentRoomWalls` physics vector, instantly trapping the player and blocking projectiles.
- **Camera Clamping:** `LevelManager::GetLevelBounds()` already feeds into `CameraManager::UpdateCamera()`. We will modify `GetLevelBounds()` so that it strictly returns the room's inner pixel boundaries when `LOCKED`, clamping the camera. When `CLEARED` or `IDLE`, we will return a much larger boundary to allow the camera to peek out of the doors.

## 3. Wave Spawner Integration
- **WaveManager Updates:** We will add `StartRoomWaves(int totalEnemies)` and `bool IsRoomCleared() const` to `WaveManager`.
- **Combat Loop:** When `LevelManager` locks the room, it will call `waveManager->StartRoomWaves(...)` (using Layer 1 nodes in the future, but currently using its standard spawn logic confined to the room bounds).
- **Resolution:** `LevelManager` will monitor `waveManager->IsRoomCleared()`. Once true, it transitions the room to `CLEARED`.

## 4. Unlock Sequence
- Once `CLEARED`, the dynamically injected door barriers are removed from `currentRoomWalls` (allowing exit).
- The camera clamp is released.
- (Future integration: Update minimap).

## 5. Temporary Rendering Override
- Per your request, in `TilemapRenderer::DrawRoom`, I will bypass the `tileset` rendering.
- `layer0_tiles` marked as `0` (Floor) will be drawn as solid `WHITE` rectangles.
- `layer0_tiles` marked as `1` (Wall) and `20` (Locked Doors) will be drawn as solid `BLACK` rectangles.

## Verification Plan
1. Start the mission using the "I will check their ship" dialogue option.
2. Verify the room draws as white floors and black walls.
3. Move into the center of the room to trigger the `LOCKED` state.
4. Verify the camera strictly clamps to the room boundaries and doors close.
5. Defeat the spawned wave.
6. Verify the room transitions to `CLEARED`, doors open, and camera unlocks.

> [!IMPORTANT]
> Since we only have a 1x1 test room right now, the doors won't actually lead anywhere yet. I will simulate the "doors" by leaving 2-tile wide gaps in the center of the North/South/East/West walls during `GenerateDungeon()`. When locked, these gaps will become solid. Does this approach work for you?
