#pragma once
#include "Core/State/IGameState.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/WaveManager.h"
#include "Core/Manager/GameManager.h"
#include "raylib.h"

#include "UI/EnhanceMenuUI.h"

class GameplayState : public IGameState {
public:
    GameplayState(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager);
    ~GameplayState() override = default;

    void Update(float deltaTime) override;
    void Draw() override;

    void OpenEnhanceMenu(PaladinId paladinId) { enhanceMenuUI.Open(paladinId); }
    EnhanceMenuUI& GetEnhanceMenuUI() { return enhanceMenuUI; }

private:
    TeamManager* teamManager;
    LevelManager* levelManager;
    WaveManager* waveManager;
    EnhanceMenuUI enhanceMenuUI;
};
