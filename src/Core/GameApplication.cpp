#include "Core/GameApplication.h"
#include "Core/Constants.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/InputManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/CameraManager.h"
#include "Core/Manager/DialogueManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/ParticleManager.h"
#include "Core/Manager/UltimateIntroManager.h"
#include "Core/DepthRenderItem.h"
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
#include <limits>
#include <iostream>

namespace {
    constexpr float MAX_MODAL_SCALE = 1.35f;
    constexpr const char* HUB_LEVEL_PATH = "assets/map/hub_Tile Layer 1.csv";

    Vector2 GetLevelCenter(const LevelManager& levelManager) {
        Rectangle bounds = levelManager.GetLevelBounds();
        return {
            bounds.x + bounds.width * 0.5f,
            bounds.y + bounds.height * 0.5f
        };
    }

    Camera2D CreateCenteredUICamera(float scale) {
        Camera2D camera = {};
        camera.zoom = scale;
        camera.offset = {
            (GetScreenWidth()  - Constants::GAME_WIDTH  * scale) * 0.5f,
            (GetScreenHeight() - Constants::GAME_HEIGHT * scale) * 0.5f
        };
        return camera;
    }

    Vector2 GetVirtualMousePosition(const Camera2D& camera) {
        Vector2 mousePosition = GetScreenToWorld2D(
            GetMousePosition(),
            camera
        );
        if (mousePosition.x < 0.0f ||
            mousePosition.x > Constants::GAME_WIDTH ||
            mousePosition.y < 0.0f ||
            mousePosition.y > Constants::GAME_HEIGHT) {
            return {-1.0f, -1.0f};
        }
        return mousePosition;
    }

    GameObject* FindNearestHubInteractable(
        const std::vector<GameObject*>& entities,
        Vector2 playerPosition
    ) {
        GameObject* nearest = nullptr;
        float nearestDistance = std::numeric_limits<float>::max();

        for (GameObject* entity : entities) {
            if (!entity) continue;

            bool canInteract = false;
            if (entity->GetObjectType() == GameObjectType::HubPaladinStand) {
                HubPaladinStand* stand = static_cast<HubPaladinStand*>(entity);
                canInteract = stand->IsWithinInteractionRange(playerPosition);
            } else if (entity->GetObjectType() == GameObjectType::NPC) {
                canInteract = Vector2Distance(playerPosition, entity->GetPosition()) < 50.0f;
            }

            if (!canInteract) continue;

            float distance = Vector2Distance(playerPosition, entity->GetPosition());
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearest = entity;
            }
        }
        return nearest;
    }

    void DrawInteractionPrompt(const std::string& text) {
        float textWidth = UIUtils::MeasureText("PixeloidSans", text, UIUtils::FontSize::SMALL).x;
        Rectangle background = {
            (Constants::GAME_WIDTH - textWidth) * 0.5f - 10.0f,
            Constants::GAME_HEIGHT - 44.0f,
            textWidth + 20.0f,
            28.0f
        };
        UIUtils::DrawPanel(background, Color{15, 20, 29, 220});
        UIUtils::DrawCenteredText("PixeloidSans", text, { background.x + background.width * 0.5f, background.y + background.height * 0.5f }, UIUtils::FontSize::SMALL, RAYWHITE);
    }

    void DrawHubInteractionPrompt(GameObject* interactable) {
        if (!interactable) return;

        std::string text = "Press F to talk";
        if (interactable->GetObjectType() == GameObjectType::HubPaladinStand) {
            HubPaladinStand* stand = static_cast<HubPaladinStand*>(interactable);
            text = std::string("Press F to inspect ") + stand->GetDisplayName();
        }

        DrawInteractionPrompt(text);
    }
}

GameApplication::GameApplication() 
    : teamManager(nullptr), 
      quitRequested(false), 
      systemInitialized(false),
      hasContinuableSession(false),
      settingsReturnState(GameState::MAIN_MENU),
      continueState(GameState::HUB),
      continueMusicName("bgm_story_mode") {
}

GameApplication::~GameApplication() {
}

void GameApplication::Initialize() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(
        Constants::SCREEN_WIDTH,
        Constants::SCREEN_HEIGHT,
        Constants::GAME_TITLE
    );
    SetExitKey(KEY_NULL);

    AudioManager::GetInstance().Initialize();
    ParticleManager::GetInstance().Initialize();
    DialogueManager::GetInstance().InitializeAssets();
    AssetManager::GetInstance().LoadGlobalFonts();
    AssetManager::GetInstance().LoadCommonAssets();

    levelManager.InitializeAssets();  // Must be after InitWindow()

    mainMenu.Initialize();
    AssetManager::GetInstance().QueueCharacterAssets();

    roomEditor.Initialize();
    
    CameraManager::GetInstance().Initialize();
    GameManager& gameManager = GameManager::GetInstance();
    gameManager.UpdateTargetFPS(Constants::TARGET_FPS);
}

void GameApplication::Shutdown() {
    AssetManager::GetInstance().UnloadAll();
    CloseWindow();
}

void GameApplication::StartNewGame() {
    GameManager::GetInstance().ResetTransientState();
    ParticleManager::GetInstance().Clear();
    DialogueManager::GetInstance().ResetSession();
    GameManager::GetInstance().ResetFloorCount();
    levelManager.LoadLevel(HUB_LEVEL_PATH, teamManager.get());
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
    levelManager.GenerateDungeon(teamManager.get());
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
    levelManager.LoadLevel(
        "assets/map/demo-big_Tile Layer 1.csv",
        teamManager.get()
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
    levelManager.LoadLevel(HUB_LEVEL_PATH, teamManager.get());
    teamManager->GetActivePaladin()->SetPosition(
        GetLevelCenter(levelManager)
    );
    AudioManager::GetInstance().PlayMusicTrack("bgm_story_mode", 1.0f);
    GameManager::GetInstance().SetState(GameState::HUB);
}

void GameApplication::RunLoop() {
    GameManager& gameManager = GameManager::GetInstance();
    
    while (!WindowShouldClose() && !quitRequested) {
        InputManager::Update();
        float deltaTime = GetFrameTime();
        
        const float viewportScale = std::min(
            (float)GetScreenWidth() / Constants::GAME_WIDTH,
            (float)GetScreenHeight() / Constants::GAME_HEIGHT
        );
        const float modalScale = std::min(
            viewportScale,
            MAX_MODAL_SCALE
        );
        const Camera2D uiCamera = CreateCenteredUICamera(viewportScale);
        const Camera2D modalCamera = CreateCenteredUICamera(modalScale);

        AudioManager::GetInstance().UpdateMusicStream();

        Vector2 uiMousePosition = GetVirtualMousePosition(uiCamera);
        Vector2 modalMousePosition = GetVirtualMousePosition(modalCamera);

        GameState state = gameManager.GetState();

        if (state == GameState::MAIN_MENU &&
            mainMenu.IsReady() &&
            !systemInitialized) {
            gameManager.SetBulletImpactTexture(
                AssetManager::GetInstance().GetTexture("Lance_Impact")
            );

            Vector2 startPosition = {0.0f, 0.0f};
            teamManager = std::make_unique<TeamManager>();
            teamManager->AddMember(new Lance(
                startPosition,
                AssetManager::GetInstance().GetLanceSprites()
            ));
            teamManager->AddMember(new Keith(
                startPosition,
                AssetManager::GetInstance().GetKeithSprites()
            ));
            teamManager->AddMember(new Hunk(
                startPosition,
                AssetManager::GetInstance().GetHunkSprites()
            ));
            teamManager->AddMember(new Pidge(
                startPosition,
                AssetManager::GetInstance().GetPidgeSprites()
            ));

            uiManager.Initialize();
            uiManager.SetTeamManager(teamManager.get());
            teamManager->RefreshAimStrategies();

            levelManager.LoadLevel(
                HUB_LEVEL_PATH,
                teamManager.get()
            );
            teamManager->ResetForNewGame(GetLevelCenter(levelManager));
            gameManager.SetLevelManager(&levelManager);
            AudioManager::GetInstance().PlayMusicTrack("bgm_starter_menu", 1.0f);
            systemInitialized = true;
        }

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
                uiManager.IsPauseButtonPressed(uiMousePosition);

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
            switch(state) {
                case GameState::HUB:
                    newState = std::make_unique<HubState>(teamManager.get(), &levelManager, &waveManager, &paladinSelectionMenu);
                    break;
                case GameState::GAMEPLAY:
                    newState = std::make_unique<GameplayState>(teamManager.get(), &levelManager, &waveManager);
                    break;
                case GameState::MAIN_MENU:
                    newState = std::make_unique<MainMenuState>(&mainMenu, this);
                    break;
                case GameState::ROOM_EDITOR:
                    newState = std::make_unique<RoomEditorStateAdapter>(&roomEditor);
                    break;
                case GameState::PAUSE:
                    newState = std::make_unique<PauseState>(&pauseMenu, this, gameManager.TakeCurrentStateObj());
                    break;
                case GameState::SETTINGS:
                    newState = std::make_unique<SettingsState>(&settingsMenu, this, gameManager.TakeCurrentStateObj());
                    break;
                case GameState::GAME_OVER:
                    newState = std::make_unique<GameOverState>(this, gameManager.TakeCurrentStateObj());
                    break;
                case GameState::VICTORY:
                    newState = std::make_unique<VictoryState>(this, gameManager.TakeCurrentStateObj());
                    break;
            }
            gameManager.SetCurrentStateObj(std::move(newState));
            previousEnumState = state;
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
            teamManager.get(),
            state
        );

        BeginDrawing();
        ClearBackground(BLACK);

        if (auto* stateObj = gameManager.GetCurrentStateObj()) {
            stateObj->Draw();
        }

        // Global HUD layer — rendered on top of all states
        bool hubModalOpenLocal = state == GameState::HUB && paladinSelectionMenu.IsOpen();
        if (systemInitialized && (state == GameState::HUB || state == GameState::GAMEPLAY) && !hubModalOpenLocal) {
            BeginMode2D(uiCamera);
            uiManager.DrawHUD(GetScreenWidth(), GetScreenHeight(), uiMousePosition);
            EndMode2D();
        }

        adminPanel.Draw();

        EndDrawing();
    }
}
