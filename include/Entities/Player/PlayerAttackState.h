#pragma once
#include "Entities/Player/PlayerState.h"
#include "raylib.h"

class Player;

class PlayerAttackState : public IPlayerState {
private:
    float attackTimer;

public:
    void Enter(Player* player) override;
    void Update(Player* player, float deltaTime) override;
    void Exit(Player* player) override;
};
