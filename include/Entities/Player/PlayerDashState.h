#pragma once
#include "Entities/Player/PlayerState.h"
#include "raylib.h"

class Player; // Forward declaration

class PlayerDashState : public IPlayerState {
private:
    Vector2 dashDirection;

public:
    void Enter(Player* player) override;
    void Update(Player* player, float deltaTime) override;
    void Exit(Player* player) override;
};
