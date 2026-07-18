# Implementation Plan: ZZZ Team HUD Refactor

## Goal Description
Update the UI Manager to accurately reflect the new 3-person team architecture. The new HUD must display the active character prominently alongside their individual EX Energy and Shared Armor, while showing miniature status trackers for the off-field characters.

## Proposed Changes

### `include/UI/UIManager.h`
- [MODIFY] Remove reliance on the obsolete `IObserver` interface parameters.
- [MODIFY] Inject a direct dependency to `TeamManager*` via a new `SetTeamManager(TeamManager* tm)` method.

### `src/UI/UIManager.cpp`
- [MODIFY] **Active Character (Top Left):** Implement Raylib math to draw a 30px circular portrait frame. Render the individual HP bar (Red), EX Energy (Yellow), and Shared Armor (Blue) directly next to the active portrait.
- [MODIFY] **Off-Field Squad:** Iterate through `TeamManager::GetTeam()`, skipping the active index. Render miniature 15px portraits and tiny red HP bars vertically stacked below the main HUD.
- [MODIFY] **Pause Button:** Render a clickable 30x30 pause icon in the top right. Implement coordinate scaling (`GAME_WIDTH / WINDOW_WIDTH`) for `CheckCollisionPointRec()` to ensure mouse clicks accurately trigger `GameManager::SetState(GameState::PAUSED)`.

## Verification Plan
1. **Manual Verification:** Launch the application in a 4:3 aspect ratio window. Swap characters using TAB and verify the active portrait and health bars swap correctly in the top-left UI. Click the Pause button in the top right to verify the internal resolution scaling math accurately pauses the game.
