#include "Core/GameApplication.h"
#include "Core/Constants.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/InputManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/CameraManager.h"
#include "Core/Manager/DialogueManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/UltimateIntroManager.h"
#include "Core/DepthRenderItem.h"
#include "Core/Diagnostics/MemoryDiagnostics.h"
#include "Core/Diagnostics/FramePerformanceStats.h"
#include "Entities/Hub/HubPaladinStand.h"
#include "Entities/NPC.h"
#include "Entities/Player/Hunk.h"
#include "Entities/Player/Keith.h"
#include "Entities/Player/Lance.h"
#include "Entities/Player/Pidge.h"
#include "UI/UIUtils.h"
#include "UI/adminGUI/AdminPanel.h"
#include "Core/State/GameplayState.h"
#include "Core/State/HubState.h"
#include "Core/State/UIStates.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
    constexpr float MAX_MODAL_SCALE = 1.35f;
    constexpr const char* HUB_LEVEL_PATH = "assets/map/hub_Tile Layer 1.csv";

    bool IsPlayableSessionState(GameState state) {
        return state == GameState::HUB || state == GameState::GAMEPLAY;
    }

    bool IsOverlayState(GameState state) {
        return state == GameState::PAUSE ||
            state == GameState::SETTINGS ||
            state == GameState::GAME_OVER ||
            state == GameState::VICTORY;
    }

    const char* GetSessionMusic(GameState state) {
        return state == GameState::GAMEPLAY
            ? "bgm_battle"
            : "bgm_story_mode";
    }

    Vector2 GetLevelCenter(const LevelManager& levelManager) {
        Rectangle bounds = levelManager.GetLevelBounds();
        return {
            bounds.x + bounds.width * 0.5f,
            bounds.y + bounds.height * 0.5f
        };
    }

}

GameApplication::GameApplication() 
    : levelManager(*GameManager::GetInstance().GetLevelManager()),
      waveManager(GameManager::GetInstance().GetWaveManager()),
      teamManager(nullptr),
      quitRequested(false), 
      systemInitialized(false),
      shutdownComplete(false),
      hasContinuableSession(false),
      settingsReturnState(GameState::MAIN_MENU),
      continueState(GameState::HUB),
      continueMusicName("bgm_story_mode") {
}

GameApplication::~GameApplication() {
    try {
        Shutdown();
    } catch (...) {
        if (IsWindowReady()) CloseWindow();
    }
}

void GameApplication::Initialize() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(
        Constants::SCREEN_WIDTH,
        Constants::SCREEN_HEIGHT,
        Constants::GAME_TITLE
    );
    SetExitKey(KEY_NULL);
    MemoryDiagnostics::ResetLog();

    AssetManager& assets = AssetManager::GetInstance();
    // Fonts and the menu shell are the minimal bootstrap resources required
    // to render the real loading screen itself.
    assets.LoadGlobalFonts();
    mainMenu.Initialize();
    GameManager& gameManager = GameManager::GetInstance();
    gameManager.UpdateTargetFPS(Constants::TARGET_FPS);

    assets.BeginLoadingQueue();
    assets.QueueCommonAssets();
    assets.QueueCharacterAssets();
    assets.QueueLoadingTask("Loading audio library", []() {
        AudioManager::GetInstance().Initialize();
    });
    assets.QueueLoadingTask("Initializing visual effects", []() {
        GameManager::GetInstance().GetEffectManager().Initialize();
    });
    assets.QueueLoadingTask("Loading dialogue portraits", []() {
        DialogueManager::GetInstance().InitializeAssets();
    });
    assets.QueueLoadingTask("Initializing map textures", [this]() {
        levelManager.InitializeAssets();
    });
    assets.QueueLoadingTask("Preparing Room Editor", [this]() {
        roomEditor.Initialize();
    });
    assets.QueueLoadingTask("Initializing camera", []() {
        CameraManager::GetInstance().Initialize();
    });
    assets.QueueLoadingTask("Creating player team and HUD", [this]() {
        InitializeTeamAndUI();
    });
    assets.QueueLoadingTask("Loading HUB map", [this]() {
        InitializeHubWorld();
    });
    assets.QueueLoadingTask("Finalizing game systems", [this]() {
        FinalizeStartup();
    });
}

void GameApplication::InitializeTeamAndUI() {
    GameManager& gameManager = GameManager::GetInstance();
    gameManager.SetBulletImpactTexture(
        AssetManager::GetInstance().GetTexture("Lance_Impact")
    );

    Vector2 startPosition = { 0.0f, 0.0f };
    auto newTeamManager = std::make_unique<TeamManager>();
    newTeamManager->AddMember(std::make_unique<Lance>(
        startPosition,
        AssetManager::GetInstance().GetLanceSprites()
    ));
    newTeamManager->AddMember(std::make_unique<Keith>(
        startPosition,
        AssetManager::GetInstance().GetKeithSprites()
    ));
    newTeamManager->AddMember(std::make_unique<Hunk>(
        startPosition,
        AssetManager::GetInstance().GetHunkSprites()
    ));
    newTeamManager->AddMember(std::make_unique<Pidge>(
        startPosition,
        AssetManager::GetInstance().GetPidgeSprites()
    ));

    gameManager.SetTeamManager(std::move(newTeamManager));
    teamManager = gameManager.GetTeamManager();
    uiManager.Initialize();
    uiManager.SetTeamManager(teamManager);
    teamManager->AddObserver(&uiManager);
    teamManager->RefreshAimStrategies();
}

void GameApplication::InitializeHubWorld() {
    if (!teamManager || !teamManager->GetActivePaladin()) {
        throw std::runtime_error(
            "Cannot initialize HUB without an active player team"
        );
    }
    GameManager::GetInstance().LoadLevel(HUB_LEVEL_PATH);
    teamManager->ResetForNewGame(GetLevelCenter(levelManager));
    teamManager->NotifyObservers();
}

void GameApplication::FinalizeStartup() {
    if (!teamManager || !teamManager->GetActivePaladin() ||
        levelManager.GetLevelWidth() <= 0.0f ||
        levelManager.GetLevelHeight() <= 0.0f) {
        throw std::runtime_error(
            "Game startup completed with an invalid world session"
        );
    }
    AudioManager::GetInstance().PlayMusicTrack(
        "bgm_starter_menu",
        1.0f
    );
    AudioManager::GetInstance().PlaySoundEffect("ui_opening");
    systemInitialized = true;
    MemoryDiagnostics::Capture(
        "startup_assets_and_system_ready",
        GameManager::GetInstance()
    );
}

void GameApplication::Shutdown() {
    if (shutdownComplete) return;

    GameManager& gameManager = GameManager::GetInstance();
    MemoryDiagnostics::Capture("shutdown_begin", gameManager);
    gameManager.ResetWorld();
    gameManager.GetEffectManager().Shutdown();
    levelManager.ShutdownAssets();
    mainMenu.Shutdown();
    AssetManager::GetInstance().UnloadAll();
    MemoryDiagnostics::Capture("resources_unloaded", gameManager);
    AudioManager::GetInstance().Shutdown();
    if (IsWindowReady()) CloseWindow();
    systemInitialized = false;
    shutdownComplete = true;
}

void GameApplication::StartNewGame() {
    ClearSuspendedSession();
    GameManager::GetInstance().ResetTransientState();
    DialogueManager::GetInstance().ResetSession();
    GameManager::GetInstance().ResetFloorCount();
    GameManager::GetInstance().LoadLevel(HUB_LEVEL_PATH);
    teamManager->ResetForNewGame(GetLevelCenter(levelManager));
    waveManager.Reset(0, 0, 0);
    AudioManager::GetInstance().PlayMusicTrack("bgm_story_mode", 1.0f);
    GameManager::GetInstance().SetState(GameState::HUB);
}

void GameApplication::ResetGame() {
    teamManager->GetActivePaladin()->SetPosition({160.0f, 160.0f});
    for (auto* paladin : teamManager->GetTeam()) {
        paladin->ResetStats();
    }
    GameManager::GetInstance().ClearProjectiles();
    GameManager::GetInstance().ResetFloorCount();
    GameManager::GetInstance().GenerateDungeon();
    waveManager.Reset(0, 0, 0);
    AudioManager::GetInstance().PlayMusicTrack("bgm_battle", 1.0f);
    GameManager::GetInstance().SetState(GameState::GAMEPLAY);
}

void GameApplication::ResetDemoGame() {
    teamManager->GetActivePaladin()->SetPosition({160.0f, 160.0f});
    for (auto* paladin : teamManager->GetTeam()) {
        paladin->ResetStats();
    }
    GameManager::GetInstance().ClearProjectiles();
    GameManager::GetInstance().LoadLevel(
        "assets/map/demo-big_Tile Layer 1.csv"
    );
    waveManager.Reset(10, 0, 0);
    AudioManager::GetInstance().PlayMusicTrack("bgm_battle", 1.0f);
    GameManager::GetInstance().SetState(GameState::GAMEPLAY);
}

void GameApplication::ReturnToHub() {
    GameManager::GetInstance().ClearProjectiles();
    for (auto* paladin : teamManager->GetTeam()) {
        paladin->ResetStats();
    }
    GameManager::GetInstance().LoadLevel(HUB_LEVEL_PATH);
    teamManager->GetActivePaladin()->SetPosition(
        GetLevelCenter(levelManager)
    );
    AudioManager::GetInstance().PlayMusicTrack("bgm_story_mode", 1.0f);
    GameManager::GetInstance().SetState(GameState::HUB);
}

void GameApplication::SuspendSessionToMainMenu() {
    GameManager& gameManager = GameManager::GetInstance();
    GameState suspendedState = gameManager.GetPreviousGameState();
    if (!IsPlayableSessionState(suspendedState)) {
        ClearSuspendedSession();
        AudioManager::GetInstance().PlayMusicTrack(
            "bgm_starter_menu",
            1.0f
        );
        gameManager.SetState(GameState::MAIN_MENU);
        return;
    }

    continueState = suspendedState;
    continueMusicName = GetSessionMusic(suspendedState);
    hasContinuableSession = true;
    mainMenu.SetContinueAvailable(true);
    AudioManager::GetInstance().PlayMusicTrack(
        "bgm_starter_menu",
        1.0f
    );
    gameManager.SetState(GameState::MAIN_MENU);
}

bool GameApplication::ContinueSuspendedSession() {
    if (!hasContinuableSession ||
        !IsPlayableSessionState(continueState)) {
        return false;
    }

    GameState restoredState = continueState;
    std::string restoredMusic = continueMusicName;
    ClearSuspendedSession();
    if (!restoredMusic.empty()) {
        AudioManager::GetInstance().PlayMusicTrack(restoredMusic, 1.0f);
    }
    GameManager::GetInstance().SetState(restoredState);
    return true;
}

void GameApplication::ClearSuspendedSession() {
    hasContinuableSession = false;
    continueState = GameState::HUB;
    continueMusicName = "bgm_story_mode";
    mainMenu.SetContinueAvailable(false);
}

void GameApplication::RunLoop() {
    GameManager& gameManager = GameManager::GetInstance();
    
    while (!WindowShouldClose() && !quitRequested) {
        InputManager::Update();
        const float rawDeltaTime = GetFrameTime();
        const float deltaTime = std::clamp(rawDeltaTime, 0.0f, 0.05f);
        FramePerformanceStats::GetInstance().Update(
            rawDeltaTime,
            gameManager.GetTargetFPS()
        );
        
        const float viewportScale = std::min(
            (float)GetScreenWidth() / Constants::GAME_WIDTH,
            (float)GetScreenHeight() / Constants::GAME_HEIGHT
        );
        const float modalScale = std::min(
            viewportScale,
            MAX_MODAL_SCALE
        );
        const Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);
        Vector2 uiMousePosition = UIUtils::GetVirtualMousePosition(uiCamera);
        Rectangle windowBounds = {
            -uiCamera.offset.x / uiCamera.zoom,
            -uiCamera.offset.y / uiCamera.zoom,
            GetScreenWidth() / uiCamera.zoom,
            GetScreenHeight() / uiCamera.zoom
        };
        const Camera2D modalCamera = UIUtils::CreateCenteredUICamera(modalScale);

        AudioManager::GetInstance().UpdateMusicStream();

        Vector2 modalMousePosition = UIUtils::GetVirtualMousePosition(modalCamera);

        GameState state = gameManager.GetState();

        Vector2 mouseWorld = {0.0f, 0.0f};
        if (systemInitialized) {
            if (state != GameState::HUB &&
                paladinSelectionMenu.IsOpen()) {
                paladinSelectionMenu.Close();
            }

            bool selectionOpen =
                state == GameState::HUB &&
                paladinSelectionMenu.IsOpen();
            bool dialogueOpen =
                state == GameState::HUB &&
                DialogueManager::GetInstance().IsActive();
            bool hubModalOpen = selectionOpen || dialogueOpen;

            Paladin* activePaladin = teamManager->GetActivePaladin();
            Vector2 playerPos = activePaladin->GetPosition();
            
            mouseWorld = GetScreenToWorld2D(
                GetMousePosition(),
                CameraManager::GetInstance().GetCamera()
            );
            
            teamManager->RefreshAimStrategies();
            activePaladin->UpdateAim(mouseWorld);

            bool keyboardPauseRequested =
                !hubModalOpen && InputManager::IsPausePressed();
            bool hudPauseRequested =
                !hubModalOpen &&
                (state == GameState::HUB ||
                 state == GameState::GAMEPLAY) &&
                uiManager.IsPauseButtonPressed(windowBounds, uiMousePosition);

            if (state == GameState::PAUSE && keyboardPauseRequested) {
                gameManager.ResumeGame();
            } else if ((state == GameState::HUB ||
                        state == GameState::GAMEPLAY) &&
                       (keyboardPauseRequested || hudPauseRequested)) {
                gameManager.PauseGame();
            }
            state = gameManager.GetState();
        }

        static GameState previousEnumState = static_cast<GameState>(-1);
        if (state != previousEnumState) {
            std::unique_ptr<IGameState> newState;
            if (state == GameState::SETTINGS &&
                previousEnumState == GameState::MAIN_MENU) {
                settingsReturnState = GameState::MAIN_MENU;
            }

            if (IsOverlayState(state)) {
                if (!gameManager.HasOverlayBackgroundState()) {
                    gameManager.PreserveCurrentStateForOverlay(
                        previousEnumState
                    );
                }
                IGameState* background =
                    gameManager.GetOverlayBackgroundState();
                switch (state) {
                    case GameState::PAUSE:
                        newState = std::make_unique<PauseState>(
                            &pauseMenu, this, background
                        );
                        break;
                    case GameState::SETTINGS:
                        newState = std::make_unique<SettingsState>(
                            &settingsMenu, this, background
                        );
                        break;
                    case GameState::GAME_OVER:
                        newState = std::make_unique<GameOverState>(
                            this, background
                        );
                        break;
                    case GameState::VICTORY:
                        newState = std::make_unique<VictoryState>(
                            this, background
                        );
                        break;
                    default:
                        break;
                }
            } else if (gameManager.RestoreOverlayBackgroundState(state)) {
                // Restore the exact gameplay/menu state hidden by the overlay.
            } else {
                gameManager.ClearOverlayBackgroundState();
                switch(state) {
                case GameState::HUB:
                    newState = std::make_unique<HubState>(teamManager, &levelManager, &waveManager, &paladinSelectionMenu);
                    break;
                case GameState::GAMEPLAY:
                    newState = std::make_unique<GameplayState>(teamManager, &levelManager, &waveManager);
                    break;
                case GameState::MAIN_MENU:
                    newState = std::make_unique<MainMenuState>(&mainMenu, this);
                    break;
                case GameState::ROOM_EDITOR:
                    newState = std::make_unique<RoomEditorStateAdapter>(&roomEditor);
                    break;
                default:
                    break;
                }
            }
            if (newState) {
                gameManager.SetCurrentStateObj(std::move(newState));
            }
            previousEnumState = state;
            MemoryDiagnostics::Capture("state_changed", gameManager);
        }

        if (auto* stateObj = gameManager.GetCurrentStateObj()) {
            stateObj->Update(deltaTime);
        }

        if (systemInitialized && teamManager && teamManager->GetActivePaladin()) {
            Vector2 aimVec = teamManager->GetActivePaladin()->GetCurrentAimVector();
            Vector2 targetPos = { teamManager->GetActivePaladin()->GetPosition().x + aimVec.x * 100.0f, teamManager->GetActivePaladin()->GetPosition().y + aimVec.y * 100.0f };
            CameraManager::GetInstance().UpdateCamera(
                teamManager->GetActivePaladin()->GetPosition(),
                targetPos,
                deltaTime,
                levelManager.GetLevelBounds(),
                gameManager.GetHitstopTimer() > 0.0f
            );
        }

        adminPanel.Update(
            GetScreenToWorld2D(
                GetMousePosition(),
                CameraManager::GetInstance().GetCamera()
            ),
            levelManager,
            teamManager,
            state
        );

        BeginDrawing();
        ClearBackground(BLACK);

        if (auto* stateObj = gameManager.GetCurrentStateObj()) {
            stateObj->Draw();
        }

        // Global HUD layer — rendered on top of all states (hidden when selection/enhance modals are open)
        bool modalOpen = (state == GameState::HUB && paladinSelectionMenu.IsOpen()) ||
                         (state == GameState::GAMEPLAY && gameManager.IsEnhanceMenuOpen());
        if (systemInitialized && (state == GameState::HUB || state == GameState::GAMEPLAY) && !modalOpen) {
            BeginMode2D(uiCamera);
            uiManager.DrawHUD(windowBounds, uiMousePosition);
            EndMode2D();
        }

        adminPanel.Draw();

        EndDrawing();
        MemoryDiagnostics::UpdatePeriodic(deltaTime, gameManager);
    }
}
