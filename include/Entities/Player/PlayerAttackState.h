#pragma once
#include "Entities/Player/PlayerState.h"
#include "raylib.h"

class Paladin;

class PlayerAttackState : public IPlayerState {
private:
    float attackTimer;

public:
    /// Prepares this state when it becomes active.
    void Enter(Paladin* player) override;
    /// Advances this component's state for the current frame.
    void Update(Paladin* player, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Paladin* player) override;
};
