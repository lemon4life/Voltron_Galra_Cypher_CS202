#pragma once
#include "Entities/Player/PlayerState.h"
#include "raylib.h"

class Paladin; // Forward declaration

class PlayerDashState : public IPlayerState {
private:
    Vector2 dashDirection;
    float trailTimer = 0.0f; // Controls how often a ghost frame is emitted

public:
    /// Prepares this state when it becomes active.
    void Enter(Paladin* player) override;
    /// Advances this component's state for the current frame.
    void Update(Paladin* player, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Paladin* player) override;
};
