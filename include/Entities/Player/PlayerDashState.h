#pragma once
#include "Entities/Player/PlayerState.h"
#include "raylib.h"

class Paladin; // Forward declaration

class PlayerDashState : public IPlayerState {
private:
    Vector2 dashDirection;

public:
    void Enter(Paladin* player) override;
    void Update(Paladin* player, float deltaTime) override;
    void Exit(Paladin* player) override;
};
