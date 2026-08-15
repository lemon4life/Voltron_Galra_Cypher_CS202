#pragma once
#include "Core/State/IGameState.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/WaveManager.h"
#include "UI/PaladinSelectionMenu.h"
#include "raylib.h"

class HubState : public IGameState {
public:
    HubState(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager, PaladinSelectionMenu* paladinSelectionMenu);
    ~HubState() override = default;

    void Update(float deltaTime) override;
    void Draw() override;

private:
    TeamManager* teamManager;
    LevelManager* levelManager;
    WaveManager* waveManager;
    PaladinSelectionMenu* paladinSelectionMenu;
};
