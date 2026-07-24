# Parry & Hitstop Implementation Plan

The objective is to implement a responsive parry state for Lance, complete with an orthogonal weapon block animation, followed by a dramatic global "hitstop" and dynamic camera zoom effect upon a successful parry.

## Proposed Changes

### 1. Global Hitstop Architecture
To achieve the hitstop, we must pause the progression of game physics and logic without pausing the rendering loop or the internal clock.
#### [MODIFY] `include/Core/Manager/GameManager.h` & `src/Core/Manager/GameManager.cpp`
- Add `float hitstopTimer = 0.0f;`.
- Add `void TriggerHitstop(float duration) { hitstopTimer = duration; }`.
- Add `float GetHitstopTimer() const { return hitstopTimer; }`.
- Add `void UpdateHitstop(float dt) { if (hitstopTimer > 0.0f) hitstopTimer -= dt; }`.

#### [MODIFY] `src/main.cpp`
- In the `GameState::PLAYING` update block, check if `GameManager::GetInstance().GetHitstopTimer() > 0.0f`.
  - If **true**: Call `UpdateHitstop(deltaTime)`. Do **not** update `levelManager`, `teamManager`, `projectiles`, or `waveManager`.
  - If **false**: Run the normal update loops.
- Apply the dynamic camera zoom during rendering:
  - `camera.zoom = GameManager::GetInstance().GetHitstopTimer() > 0.0f ? 2.2f : 2.0f;`

### 2. Player Parry State
#### [MODIFY] `include/Entities/Player/PlayerState.h` & `src/Entities/Player/PlayerState.cpp`
- Implement `PlayerParryState : public IPlayerState`.
- **Enter**: Set player velocity to `0`, start `parryTimer = 0.2f`.
- **Update**: Decrement timer. If `parryTimer <= 0.0f`, transition back to `PlayerIdleState`.

#### [MODIFY] `include/Entities/Player/Paladin.h` & `src/Entities/Player/Paladin.cpp`
- Add `Texture2D parryTex` to `CharacterSprites`, loaded in `main.cpp`.
- Add `bool IsParrying() const`.
- Add `void TriggerParrySuccess(GameObject* attacker)`.
- In `Paladin::Draw()`: If `IsParrying()` is true, draw `parryTex`.
- If `attacker` is known (passed during `TriggerParrySuccess`), calculate the angle to the attacker, add 90 degrees (orthogonal), and rotate the `parryTex` to face that angle.

#### [MODIFY] `src/Entities/Player/Lance.cpp`
- Add input detection for `KEY_F`. If pressed and cooldown/stamina allows, `ChangeState(new PlayerParryState())`. *(Note: This can be added to the base Paladin input polling if it applies to all characters).*

### 3. Enemy Collision & Hitstop Trigger
#### [MODIFY] `src/AI/EnemyState.cpp` (specifically `EnemyChaseState::Update`)
- When an enemy's bounding box overlaps the active paladin's bounding box:
  - Check if the paladin is currently parrying (`IsParrying()`).
  - If **true**: Call `paladin->TriggerParrySuccess(enemy)`. Inside that function, call `GameManager::GetInstance().TriggerHitstop(0.3f)`. Apply massive knockback to the enemy and set a long attack cooldown to simulate a stun.
  - If **false**: Proceed with normal `TakeDamage` logic.

## User Review Required
> [!IMPORTANT]
> - Currently, the parry logic only checks for physical overlap with an enemy's body (since enemies do contact damage). Do we need to parry projectiles as well for this iteration?
> - `TriggerHitstop` halts all logic. This means particle effects and UI timers (like dialogue) will freeze for 0.3 seconds during the hitstop. This is usually desired for dramatic effect, but please confirm.

## Verification Plan
1. Launch game, select Lance.
2. Allow an enemy to approach.
3. Press `F` to trigger the 0.2s parry window.
4. Verify the weapon is rendered orthogonally to the attacker upon successful parry.
5. Verify the entire game freezes momentarily (hitstop) and the camera zooms in slightly, before resuming normal gameplay.
