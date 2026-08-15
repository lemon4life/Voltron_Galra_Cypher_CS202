#pragma once
#include "Core/State/IGameState.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/WaveManager.h"
#include "Core/Manager/GameManager.h"
#include "raylib.h"

class GameplayState : public IGameState {
public:
    GameplayState(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager);
    ~GameplayState() override = default;

    void Update(float deltaTime) override;
    void Draw() override;

private:
    TeamManager* teamManager;
    LevelManager* levelManager;
    WaveManager* waveManager;
};
