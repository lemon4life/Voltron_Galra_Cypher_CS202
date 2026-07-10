# Player State Structure

## Goal
Implement the Player character utilizing the State Design Pattern to cleanly separate movement, idle, and dashing behaviors, making the entity easy to extend.

## Proposed Changes

### 1. State Interface
**[NEW] include/Entities/PlayerState.h & src/Entities/PlayerState.cpp**
- Define `IPlayerState` interface with:
  - `virtual void Enter(Player* player) = 0;`
  - `virtual void Update(Player* player, float deltaTime) = 0;`
  - `virtual void Exit(Player* player) = 0;`

### 2. Concrete States
- **PlayerIdleState**: Monitors WASD/Arrow keys. If pressed, calls `player->ChangeState(player->GetRunState())`.
- **PlayerRunState**: Handles 4-directional movement (normalizing diagonals so speed doesn't exceed `player->GetSpeed()`). If keys are released, transitions to `IdleState`. If Space is pressed and dash is off cooldown, transitions to `DashState`.
- **PlayerDashState**: Applies a speed multiplier for a fixed duration (`0.2f` seconds), ignoring input, then returns to `RunState`.

### 3. Player Class
**[NEW] include/Entities/Player.h & src/Entities/Player.cpp**
- Inherits from `Character`.
- **State Management**: Holds pointers to `IPlayerState* currentState`, and instantiates the concrete states internally.
- **Update Logic**: `void Update(float deltaTime)` simply delegates to `currentState->Update(this, deltaTime)`.
- **Assets**: Loads Lance and Keith sprite sheets (Idle, Run) and draws the correct frame based on the current state and a timer.

## Verification Plan
### Automated Tests
- No automated tests required for states.

### Manual Verification
- Move the player in all 8 directions to ensure speed is consistent and diagonal movement is normalized.
- Verify that pressing Space triggers the dash and correctly returns to running or idle state.
- Ensure the sprite animations align with the correct state (Idle vs Run).
