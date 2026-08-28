#pragma once
#include "Core/IObserver.h"
#include "raylib.h"
#include <vector>

class TeamManager;

class UIManager : public IObserver {
private:
    TeamManager* teamManager;
    Texture2D statsShell;
    Texture2D statsShellBack;

    // Observer cached stats from TeamManager
    std::vector<PlayerStatsSnapshot> cachedPlayerStats;
    TeamStatsSnapshot cachedTeamStats;
    bool hasReceivedStats = false;

public:
    UIManager();
    ~UIManager() override;

    void Initialize();
    void SetTeamManager(TeamManager* tm) { teamManager = tm; }
    bool IsPauseButtonPressed(Rectangle windowBounds, Vector2 mousePosition) const;

    // IObserver interface implementation
    void OnPlayerStatsChanged(const PlayerStatsSnapshot& stats, int slotIndex) override;
    void OnTeamStatsChanged(const TeamStatsSnapshot& stats) override;
    
    void DrawTeamHUD(
        TeamManager* team,
        Rectangle windowBounds,
        Vector2 mousePosition
    );
    void DrawHUD(
        Rectangle windowBounds,
        Vector2 mousePosition
    );
    void DrawCoinHUD(
        Rectangle bounds,
        int coins
    );

    // --- Core UI Helpers ---
    // Draws a full-screen semi-transparent black rectangle over previous renders
    static void DrawModalOverlay();

    // Draws a reusable pop-up card frame centered in the screen or at given bounds
    static void DrawPopupFrame(Rectangle bounds, const char* title);
};
