#pragma once
#include "Entities/Player/PlayerState.h"
#include "raylib.h"

class Paladin;

class PlayerAttackState : public IPlayerState {
private:
    float attackTimer;

public:
    void Enter(Paladin* player) override;
    void Update(Paladin* player, float deltaTime) override;
    void Exit(Paladin* player) override;
};
