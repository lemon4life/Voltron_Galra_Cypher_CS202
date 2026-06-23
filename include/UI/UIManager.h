#pragma once
#include "Core/IObserver.h"
#include "raylib.h"

class UIManager : public IObserver {
private:
    int currentHp;
    int maxHp;
    int currentArmor;
    int maxArmor;
    bool isLance;

public:
    UIManager();
    ~UIManager() override = default;

    void OnPlayerStatsChanged(int hp, int maxHp, int armor, int maxArmor, bool isLance) override;
    
    void DrawHUD(int screenWidth, int screenHeight);
};
