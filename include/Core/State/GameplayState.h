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
    /// Creates a GameplayState instance from the supplied configuration.
    GameplayState(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager);
    /// Releases resources owned by this GameplayState instance.
    ~GameplayState() override = default;

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;

    /// Opens enhance menu.
    void OpenEnhanceMenu(PaladinId paladinId) { enhanceMenuUI.Open(paladinId); }
    /// Returns the current enhance menu ui.
    EnhanceMenuUI& GetEnhanceMenuUI() { return enhanceMenuUI; }

private:
    TeamManager* teamManager;
    LevelManager* levelManager;
    WaveManager* waveManager;
    EnhanceMenuUI enhanceMenuUI;
};
