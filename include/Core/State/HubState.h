#pragma once
#include "Core/State/IGameState.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/WaveManager.h"
#include "UI/PaladinSelectionMenu.h"
#include "raylib.h"

class HubState : public IGameState {
public:
    /// Creates a HubState instance from the supplied configuration.
    HubState(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager, PaladinSelectionMenu* paladinSelectionMenu);
    /// Releases resources owned by this HubState instance.
    ~HubState() override = default;

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;

private:
    TeamManager* teamManager;
    LevelManager* levelManager;
    WaveManager* waveManager;
    PaladinSelectionMenu* paladinSelectionMenu;
};
