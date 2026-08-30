#pragma once

#include <memory>
#include <string>

#include "raylib.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/WaveManager.h"
#include "Core/Level/RoomEditorState.h"
#include "UI/MainMenu.h"
#include "UI/PauseMenu.h"
#include "UI/PaladinSelectionMenu.h"
#include "UI/SettingsMenu.h"
#include "UI/UIManager.h"
#include "UI/adminGUI/AdminPanel.h"

class GameApplication {
public:
    /// Creates a GameApplication instance from the supplied configuration.
    GameApplication();
    /// Releases resources owned by this GameApplication instance.
    ~GameApplication();

    /// Initializes the resources and collaborators required before this component can run.
    void Initialize();
    /// Runs the main frame loop until the window or an in-game quit action closes it.
    /// It coordinates input, state transitions, updates, rendering, audio, and diagnostics.
    void RunLoop();
    /// Releases resources owned by this component and leaves it safe to destroy.
    void Shutdown();

    /// Clears prior session state and starts a fresh game from the Hub.
    void StartNewGame();
    /// Resets game.
    void ResetGame();
    /// Resets demo game.
    void ResetDemoGame();
    /// Implements the return to hub behavior for this component.
    void ReturnToHub();
    /// Preserves the resumable session state before returning to the main menu.
    void SuspendSessionToMainMenu();
    /// Restores a suspended session and its matching music when one is available.
    bool ContinueSuspendedSession();
    /// Clears suspended session.
    void ClearSuspendedSession();

    friend class MainMenuState;
    friend class PauseState;
    friend class SettingsState;
    friend class GameOverState;
    friend class VictoryState;

private:
    /// Initializes team and ui.
    void InitializeTeamAndUI();
    /// Initializes hub world.
    void InitializeHubWorld();
    /// Finalizes startup.
    void FinalizeStartup();

    MainMenu mainMenu;
    PauseMenu pauseMenu;
    SettingsMenu settingsMenu;
    PaladinSelectionMenu paladinSelectionMenu;
    AdminPanel adminPanel;
    RoomEditorState roomEditor;
    
    UIManager uiManager;
    LevelManager& levelManager;
    WaveManager& waveManager;
    TeamManager* teamManager;

    bool quitRequested;
    bool systemInitialized;
    bool shutdownComplete;
    bool hasContinuableSession;
    
    GameState settingsReturnState;
    GameState continueState;
    std::string continueMusicName;
};
