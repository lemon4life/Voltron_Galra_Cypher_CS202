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
    GameApplication();
    ~GameApplication();

    void Initialize();
    void RunLoop();
    void Shutdown();

    void StartNewGame();
    void ResetGame();
    void ResetDemoGame();
    void ReturnToHub();

    friend class MainMenuState;
    friend class PauseState;
    friend class SettingsState;
    friend class GameOverState;
    friend class VictoryState;

private:

    MainMenu mainMenu;
    PauseMenu pauseMenu;
    SettingsMenu settingsMenu;
    PaladinSelectionMenu paladinSelectionMenu;
    AdminPanel adminPanel;
    RoomEditorState roomEditor;
    
    UIManager uiManager;
    LevelManager levelManager;
    WaveManager waveManager;
    std::unique_ptr<TeamManager> teamManager;

    bool quitRequested;
    bool systemInitialized;
    bool hasContinuableSession;
    
    GameState settingsReturnState;
    GameState continueState;
    std::string continueMusicName;
};
