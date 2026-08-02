#pragma once
#include "Core/IObserver.h"
#include "raylib.h"

class TeamManager;

class UIManager : public IObserver {
private:
    TeamManager* teamManager;
    Texture2D statsShell;
    Texture2D statsShellBack;

public:
    UIManager();
    ~UIManager() override;

    void Initialize();
    void SetTeamManager(TeamManager* tm) { teamManager = tm; }
    bool IsPauseButtonPressed(Vector2 mousePosition) const;

    // Obsolete but kept to fulfill IObserver interface
    void OnPlayerStatsChanged(int hp, int maxHp, int armor, int maxArmor, bool isLance) override {}
    
    void DrawTeamHUD(
        TeamManager* team,
        int screenWidth,
        int screenHeight,
        Vector2 mousePosition
    );
    void DrawHUD(
        int screenWidth,
        int screenHeight,
        Vector2 mousePosition
    );

    // --- Core UI Helpers ---
    // Draws a full-screen semi-transparent black rectangle over previous renders
    static void DrawModalOverlay();

    // Draws a reusable pop-up card frame centered in the screen or at given bounds
    static void DrawPopupFrame(Rectangle bounds, const char* title);
};
