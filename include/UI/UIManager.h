#pragma once
#include "Core/IObserver.h"
#include "raylib.h"

class TeamManager;

class UIManager : public IObserver {
private:
    TeamManager* teamManager;

public:
    UIManager();
    ~UIManager() override = default;

    void SetTeamManager(TeamManager* tm) { teamManager = tm; }

    // Obsolete but kept to fulfill IObserver interface
    void OnPlayerStatsChanged(int hp, int maxHp, int armor, int maxArmor, bool isLance) override {}
    
    void DrawHUD(int screenWidth, int screenHeight);
};
