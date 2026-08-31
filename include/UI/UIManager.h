#pragma once
#include "Core/IObserver.h"
#include "raylib.h"
#include <vector>

class TeamManager;
class Boss;

// Design Pattern - Observer (Concrete Observer):
// UIManager subscribes to TeamManager and caches PlayerStatsSnapshot and
// TeamStatsSnapshot values. HUD drawing consumes those snapshots rather than
// polling and owning the underlying combat state.
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
    /// Creates a UIManager instance from the supplied configuration.
    UIManager();
    /// Releases resources owned by this UIManager instance.
    ~UIManager() override;

    /// Initializes the resources and collaborators required before this component can run.
    void Initialize();
    /// Updates the stored team manager.
    void SetTeamManager(TeamManager* tm) { teamManager = tm; }
    /// Reports whether the pause button pressed condition is satisfied.
    bool IsPauseButtonPressed(Rectangle windowBounds, Vector2 mousePosition) const;

    // IObserver interface implementation
    /// Handles the player stats changed event.
    void OnPlayerStatsChanged(const PlayerStatsSnapshot& stats, int slotIndex) override;
    /// Handles the team stats changed event.
    void OnTeamStatsChanged(const TeamStatsSnapshot& stats) override;
    
    /// Renders team hud.
    void DrawTeamHUD(
        TeamManager* team,
        Rectangle windowBounds,
        Vector2 mousePosition
    );
    /// Renders hud.
    void DrawHUD(
        Rectangle windowBounds,
        Vector2 mousePosition
    );
    /// Renders coin hud.
    void DrawCoinHUD(
        Rectangle bounds,
        int coins
    );

    /// Renders Soul Knight style boss health bar fixed at top of screen.
    static void DrawBossHealthBar(
        Boss* boss,
        Rectangle windowBounds,
        float deltaTime
    );

    // --- Core UI Helpers ---
    // Draws a full-screen semi-transparent black rectangle over previous renders
    /// Renders modal overlay.
    static void DrawModalOverlay();

};
