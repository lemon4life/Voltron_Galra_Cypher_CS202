#include "raylib.h"
#include "raymath.h"

#include "Core/Constants.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/InputManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/CameraManager.h"
#include "Core/Manager/DialogueManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/ParticleManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/WaveManager.h"
#include "Core/Manager/UltimateIntroManager.h"
#include "Core/DepthRenderItem.h"
#include "Entities/Hub/HubPaladinStand.h"
#include "Entities/NPC.h"
#include "Entities/Player/Hunk.h"
#include "Entities/Player/Keith.h"
#include "Entities/Player/Lance.h"
#include "Entities/Player/Pidge.h"
#include "UI/MainMenu.h"
#include "UI/PauseMenu.h"
#include "UI/PaladinSelectionMenu.h"
#include "UI/UIUtils.h"
#include "UI/SettingsMenu.h"
#include "UI/UIManager.h"
#include "UI/MinimapRenderer.h"
#include "UI/adminGUI/AdminPanel.h"
#include "Core/AimStrategy/MouseAimStrategy.h"
#include "Core/AimStrategy/AutoAimStrategy.h"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

namespace {
    constexpr float MAX_MODAL_SCALE = 1.35f;
    constexpr const char* HUB_LEVEL_PATH =
        "assets/map/hub_Tile Layer 1.csv";

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
            (GetScreenWidth() - Constants::GAME_WIDTH * scale) * 0.5f,
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
            if (!entity) {
                continue;
            }

            bool canInteract = false;
            if (entity->GetObjectType() == GameObjectType::HubPaladinStand) {
                HubPaladinStand* stand =
                    static_cast<HubPaladinStand*>(entity);
                canInteract = stand->IsWithinInteractionRange(playerPosition);
            } else if (entity->GetObjectType() == GameObjectType::NPC) {
                canInteract = Vector2Distance(
                    playerPosition,
                    entity->GetPosition()
                ) < 50.0f;
            }

            if (!canInteract) {
                continue;
            }

            float distance = Vector2Distance(
                playerPosition,
                entity->GetPosition()
            );
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearest = entity;
            }
        }
        return nearest;
    }

    void DrawHubInteractionPrompt(GameObject* interactable) {
        if (!interactable) {
            return;
        }

        std::string text = "Press F to talk";
        if (interactable->GetObjectType() == GameObjectType::HubPaladinStand) {
            HubPaladinStand* stand = static_cast<HubPaladinStand*>(interactable);
            text = std::string("Press F to inspect ") + 
                   stand->GetDisplayName();
        }

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

    void ResetGame(
        TeamManager* teamManager,
        LevelManager* levelManager,
        WaveManager* waveManager
    ) {
        teamManager->GetActivePaladin()->SetPosition({160.0f, 160.0f});
        for (auto* paladin : teamManager->GetTeam()) {
            paladin->ResetStats();
        }
        GameManager::GetInstance().ClearProjectiles();
        levelManager->GenerateDungeon(teamManager);
        waveManager->Reset(0, 0, 0);
        AudioManager::GetInstance().PlayMusicTrack("bgm_battle", 1.0f);
        GameManager::GetInstance().SetState(GameState::GAMEPLAY);
    }

    void ResetDemoGame(
        TeamManager* teamManager,
        LevelManager* levelManager,
        WaveManager* waveManager
    ) {
        teamManager->GetActivePaladin()->SetPosition({160.0f, 160.0f});
        for (auto* paladin : teamManager->GetTeam()) {
            paladin->ResetStats();
        }
        GameManager::GetInstance().ClearProjectiles();
        levelManager->LoadLevel(
            "assets/map/demo-big_Tile Layer 1.csv",
            teamManager
        );
        waveManager->Reset(10, 0, 0);
        AudioManager::GetInstance().PlayMusicTrack("bgm_battle", 1.0f);
        GameManager::GetInstance().SetState(GameState::GAMEPLAY);
    }

    void OpenMainMenu() {
        AudioManager::GetInstance().PlayMusicTrack("bgm_starter_menu", 1.0f);
        GameManager::GetInstance().SetState(GameState::MAIN_MENU);
    }

    void StartNewGame(
        TeamManager* teamManager,
        LevelManager* levelManager,
        WaveManager* waveManager
    ) {
        GameManager::GetInstance().ResetTransientState();
        ParticleManager::GetInstance().Clear();
        DialogueManager::GetInstance().ResetSession();
        levelManager->LoadLevel(HUB_LEVEL_PATH, teamManager);
        teamManager->ResetForNewGame(GetLevelCenter(*levelManager));
        waveManager->Reset(0, 0, 0);
        AudioManager::GetInstance().PlayMusicTrack("bgm_story_mode", 1.0f);
        GameManager::GetInstance().SetState(GameState::HUB);
    }

    void ReturnToHub(
        TeamManager* teamManager,
        LevelManager* levelManager
    ) {
        GameManager::GetInstance().ClearProjectiles();
        for (auto* paladin : teamManager->GetTeam()) {
            paladin->ResetStats();
        }
        levelManager->LoadLevel(HUB_LEVEL_PATH, teamManager);
        teamManager->GetActivePaladin()->SetPosition(
            GetLevelCenter(*levelManager)
        );
        AudioManager::GetInstance().PlayMusicTrack("bgm_story_mode", 1.0f);
        GameManager::GetInstance().SetState(GameState::HUB);
    }
}

int main() {
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

    MainMenu mainMenu;
    mainMenu.Initialize();
    AssetManager::GetInstance().QueueCharacterAssets();

    PauseMenu pauseMenu;
    SettingsMenu settingsMenu;
    PaladinSelectionMenu paladinSelectionMenu;
    AdminPanel adminPanel;
    bool quitRequested = false;

    TeamManager* teamManager = nullptr;
    UIManager uiManager;
    LevelManager levelManager;
    WaveManager waveManager;
    bool systemInitialized = false;

    CameraManager::GetInstance().Initialize();
    GameManager& gameManager = GameManager::GetInstance();
    gameManager.UpdateTargetFPS(Constants::TARGET_FPS);

    GameState settingsReturnState = GameState::MAIN_MENU;
    GameState continueState = GameState::HUB;
    std::string continueMusicName = "bgm_story_mode";
    bool hasContinuableSession = false;
    MouseAimStrategy mouseStrategy;
    AutoAimStrategy autoStrategy;

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
            teamManager = new TeamManager();
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
            uiManager.SetTeamManager(teamManager);

            levelManager.LoadLevel(
                HUB_LEVEL_PATH,
                teamManager
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
            if (!hubModalOpen) {
                teamManager->GetActivePaladin()->SetAimTarget(mouseWorld);
            }

            Paladin* activePaladin = teamManager->GetActivePaladin();
            Vector2 playerPos = activePaladin->GetPosition();
            Vector2 unifiedAimTarget = playerPos;
            
            if (Constants::isAutoAimEnabled) {
                float bestDist = 400.0f; // Targeting range
                Enemy* bestTarget = nullptr;
                
                for (auto* entity : gameManager.GetLevelEntities()) {
                    if (entity->GetObjectType() == GameObjectType::Enemy) {
                        Enemy* enemy = static_cast<Enemy*>(entity);
                        if (enemy->IsDead()) continue;
                        
                        Vector2 toEnemy = {enemy->GetPosition().x - playerPos.x, enemy->GetPosition().y - playerPos.y};
                        float dist = Vector2Length(toEnemy);
                        
                        // 360 degree search, closest enemy priority
                        if (dist < bestDist) {
                            bestDist = dist;
                            bestTarget = enemy;
                        }
                    }
                }
                
                if (bestTarget) {
                    activePaladin->SetLockedEnemy(bestTarget);
                } else {
                    activePaladin->SetLockedEnemy(nullptr);
                }
                
                // Camera will track the interpolated aim vector directly to decouple from raw mouse coords
                Vector2 aimVec = { cosf(activePaladin->GetCurrentAimAngle()), sinf(activePaladin->GetCurrentAimAngle()) };
                mouseWorld = { playerPos.x + aimVec.x * 100.0f, playerPos.y + aimVec.y * 100.0f };
            } else {
                // Manual Aim OFF: Compute screen-to-world mouse coordinates
                mouseWorld = GetScreenToWorld2D(
                    GetMousePosition(),
                    CameraManager::GetInstance().GetCamera()
                );
                unifiedAimTarget = mouseWorld;
                activePaladin->SetLockedEnemy(nullptr);
            }
            
            activePaladin->SetAimTarget(unifiedAimTarget);

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

        switch (state) {
            case GameState::MAIN_MENU: {
                mainMenu.Update(deltaTime);
                MainMenuAction menuAction = mainMenu.ConsumeAction();
                if (menuAction == MainMenuAction::StartGame &&
                    systemInitialized) {
                    paladinSelectionMenu.Close();
                    hasContinuableSession = false;
                    mainMenu.SetContinueAvailable(false);
                    StartNewGame(
                        teamManager,
                        &levelManager,
                        &waveManager
                    );
                } else if (menuAction == MainMenuAction::Continue &&
                           hasContinuableSession) {
                    hasContinuableSession = false;
                    mainMenu.SetContinueAvailable(false);
                    if (!continueMusicName.empty()) {
                        AudioManager::GetInstance().PlayMusicTrack(
                            continueMusicName,
                            1.0f
                        );
                    }
                    gameManager.SetState(continueState);
                }
                if (mainMenu.ConsumeQuitRequest()) {
                    quitRequested = true;
                }
                if (gameManager.GetState() == GameState::SETTINGS) {
                    settingsReturnState = GameState::MAIN_MENU;
                }
                if (systemInitialized && IsKeyPressed(KEY_R)) {
                    paladinSelectionMenu.Close();
                    ResetDemoGame(teamManager, &levelManager, &waveManager);
                }
                break;
            }
            case GameState::HUB:
                if (!systemInitialized) {
                    break;
                }
                if (paladinSelectionMenu.IsOpen()) {
                    paladinSelectionMenu.Update(
                        deltaTime,
                        uiMousePosition,
                        *teamManager
                    );
                } else if (DialogueManager::GetInstance().IsActive()) {
                    DialogueManager::GetInstance().Update(deltaTime);
                } else {
                    if (DialogueManager::GetInstance().IsMissionRequested()) {
                        int missionId = DialogueManager::GetInstance().GetRequestedMissionId();
                        DialogueManager::GetInstance().ClearMissionRequest();
                        paladinSelectionMenu.Close();

                        if (missionId == -1) {
                            ResetGame(teamManager, &levelManager, &waveManager);
                        }
                        break;
                    }

                    if (Constants::isAutoAimEnabled || InputManager::GetMode() == InputMode::KEYBOARD_ONLY) {
                        teamManager->GetActivePaladin()->SetCurrentAimStrategy(&autoStrategy);
                    } else {
                        teamManager->GetActivePaladin()->SetCurrentAimStrategy(&mouseStrategy);
                    }
                    
                    levelManager.UpdateLevel(deltaTime, teamManager->GetActivePaladin()->GetPosition());
                    teamManager->Update(deltaTime);

                    if (InputManager::IsInteractPressed()) {
                        GameObject* interactable = FindNearestHubInteractable(
                            gameManager.GetLevelEntities(),
                            teamManager->GetActivePaladin()->GetPosition()
                        );
                        if (interactable &&
                            interactable->GetObjectType() ==
                                GameObjectType::HubPaladinStand) {
                            HubPaladinStand* stand =
                                static_cast<HubPaladinStand*>(interactable);
                            paladinSelectionMenu.Open(
                                stand->GetPaladinId()
                            );
                        } else if (interactable &&
                                   interactable->GetObjectType() ==
                                       GameObjectType::NPC) {
                            DialogueManager::GetInstance().LoadDialogueTree(
                                "assets/story/intro.txt"
                            );
                            DialogueManager::GetInstance().StartDialogue();
                        }
                    }
                }
                if (!paladinSelectionMenu.IsOpen()) {
                    gameManager.UpdateEffects(deltaTime);
                    CameraManager::GetInstance().UpdateCamera(
                        teamManager->GetActivePaladin()->GetPosition(),
                        mouseWorld,
                        deltaTime,
                        levelManager.GetLevelBounds(),
                        false
                    );
                }
                break;
            case GameState::GAMEPLAY:
                if (!systemInitialized) {
                    break;
                }
                if (InputManager::IsToggleAutoAimPressed()) {
                    Constants::isAutoAimEnabled = !Constants::isAutoAimEnabled;
                }
                if (UltimateIntroManager::GetInstance().IsPlaying()) {
                    UltimateIntroManager::GetInstance().Update(deltaTime);
                } else if (gameManager.GetHitstopTimer() > 0.0f) {
                    gameManager.UpdateHitstop(deltaTime);
                } else {
                    if (Constants::isAutoAimEnabled || InputManager::GetMode() == InputMode::KEYBOARD_ONLY) {
                        teamManager->GetActivePaladin()->SetCurrentAimStrategy(&autoStrategy);
                    } else {
                        teamManager->GetActivePaladin()->SetCurrentAimStrategy(&mouseStrategy);
                    }
                    
                    levelManager.UpdateLevel(deltaTime, teamManager->GetActivePaladin()->GetPosition());
                    if (levelManager.NeedsPlayerNudge()) {
                        teamManager->GetActivePaladin()->SetPosition(levelManager.ConsumeNudge());
                    }
                    teamManager->Update(deltaTime);
                    
                    if (InputManager::IsInteractPressed()) {
                        if (levelManager.IsPlayerInExitRoom(teamManager->GetActivePaladin()->GetPosition())) {
                            ReturnToHub(teamManager, &levelManager);
                            break;
                        }
                    }
                    
                    gameManager.UpdateProjectiles(deltaTime, teamManager);
                    gameManager.UpdateAssists(deltaTime, teamManager);
                    waveManager.Update(deltaTime, teamManager, &levelManager);
                }
                gameManager.UpdateEffects(deltaTime);
                ParticleManager::GetInstance().Update(deltaTime);
                CameraManager::GetInstance().UpdateCamera(
                    teamManager->GetActivePaladin()->GetPosition(),
                    mouseWorld,
                    deltaTime,
                    levelManager.GetLevelBounds(),
                    gameManager.GetHitstopTimer() > 0.0f
                );
                break;
            case GameState::PAUSE:
                switch (pauseMenu.Update(modalMousePosition)) {
                    case PauseMenuAction::Resume:
                        gameManager.ResumeGame();
                        break;
                    case PauseMenuAction::Settings:
                        settingsReturnState = GameState::PAUSE;
                        gameManager.SetState(GameState::SETTINGS);
                        break;
                    case PauseMenuAction::BackToMainMenu:
                        paladinSelectionMenu.Close();
                        continueState = gameManager.GetPreviousGameState();
                        continueMusicName = AudioManager::GetInstance()
                            .GetCurrentMusicName();
                        hasContinuableSession = true;
                        mainMenu.SetContinueAvailable(true);
                        OpenMainMenu();
                        break;
                    case PauseMenuAction::Quit:
                        quitRequested = true;
                        break;
                    case PauseMenuAction::None:
                        break;
                }
                break;
            case GameState::SETTINGS: {
                bool backRequested =
                    settingsMenu.Update(modalMousePosition);
                if (!backRequested && IsKeyPressed(KEY_ESCAPE)) {
                    AudioManager::GetInstance().PlayRandomClick();
                    backRequested = true;
                }
                if (backRequested) {
                    gameManager.SetState(settingsReturnState);
                }
                break;
            }
            case GameState::GAME_OVER:
                if (IsKeyPressed(KEY_R)) {
                    ResetGame(teamManager, &levelManager, &waveManager);
                }
                break;
            case GameState::VICTORY:
                if (IsKeyPressed(KEY_R)) {
                    paladinSelectionMenu.Close();
                    hasContinuableSession = false;
                    mainMenu.SetContinueAvailable(false);
                    OpenMainMenu();
                }
                break;
        }

        adminPanel.Update(
            GetScreenToWorld2D(
                GetMousePosition(),
                CameraManager::GetInstance().GetCamera()
            ),
            levelManager,
            teamManager,
            gameManager.GetState()
        );

        state = gameManager.GetState();
        GameState renderState = gameManager.GetRenderState();
        if (state == GameState::SETTINGS &&
            settingsReturnState == GameState::PAUSE) {
            renderState = gameManager.GetPreviousGameState();
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (renderState == GameState::HUB ||
            renderState == GameState::GAMEPLAY) {
            BeginMode2D(CameraManager::GetInstance().GetCamera());
            ClearBackground(BLACK);
            levelManager.DrawLevelBase();

            gameManager.DrawEffects(true);

            std::vector<DepthRenderItem> depthItems;
            levelManager.GetDepthRenderItems(depthItems);
            teamManager->AddDepthRenderItems(depthItems);
            
            if (renderState == GameState::GAMEPLAY) {
                gameManager.AddDepthRenderItems(depthItems);
            }

            std::sort(depthItems.begin(), depthItems.end(), [](const DepthRenderItem& a, const DepthRenderItem& b) {
                return a.ySort < b.ySort;
            });

            for (const auto& item : depthItems) {
                item.drawFunc();
            }

            ParticleManager::GetInstance().Draw();
            gameManager.DrawEffects(false);
            
            // Draw Target Indicator / Crosshair
            if (renderState == GameState::GAMEPLAY && systemInitialized) {
                if (Constants::isAutoAimEnabled) {
                    Paladin* activePaladin = teamManager->GetActivePaladin();
                    if (activePaladin && activePaladin->GetLockedEnemy()) {
                        Vector2 targetPos = activePaladin->GetLockedEnemy()->GetPosition();
                        DrawCircleLines(static_cast<int>(targetPos.x), static_cast<int>(targetPos.y), 20.0f, RED);
                        DrawLine(targetPos.x - 25, targetPos.y, targetPos.x + 25, targetPos.y, RED);
                        DrawLine(targetPos.x, targetPos.y - 25, targetPos.x, targetPos.y + 25, RED);
                    }
                } else if (InputManager::GetMode() != InputMode::KEYBOARD_ONLY) {
                    // Manual Aim Crosshair
                    DrawCircleLines(static_cast<int>(mouseWorld.x), static_cast<int>(mouseWorld.y), 10.0f, GREEN);
                    DrawLine(mouseWorld.x - 15, mouseWorld.y, mouseWorld.x + 15, mouseWorld.y, GREEN);
                    DrawLine(mouseWorld.x, mouseWorld.y - 15, mouseWorld.x, mouseWorld.y + 15, GREEN);
                    DrawCircle(static_cast<int>(mouseWorld.x), static_cast<int>(mouseWorld.y), 2.0f, GREEN);
                }
            }
            
            EndMode2D();

            BeginMode2D(uiCamera);
            if (!paladinSelectionMenu.IsOpen()) {
                uiManager.DrawHUD(
                    Constants::GAME_WIDTH,
                    Constants::GAME_HEIGHT,
                    uiMousePosition
                );
            }
            if (renderState == GameState::GAMEPLAY && UltimateIntroManager::GetInstance().IsPlaying()) {
                UltimateIntroManager::GetInstance().Draw();
            }
            if (renderState == GameState::HUB &&
                DialogueManager::GetInstance().IsActive()) {
                DialogueManager::GetInstance().Draw(
                    Constants::GAME_WIDTH,
                    Constants::GAME_HEIGHT
                );
            } else if (renderState == GameState::GAMEPLAY) {
                waveManager.DrawHUD();

                if (levelManager.IsProceduralDungeon()) {
                    int currentGridX = 3;
                    int currentGridY = 3;
                    auto paladin = teamManager->GetActivePaladin();
                    if (paladin) {
                        float tileW = Constants::TILE_SIZE * Constants::GLOBAL_SCALE;
                        int roomOuterSize = Constants::MAX_ROOM_TILE_SIZE + Constants::CORRIDOR_LENGTH;
                        currentGridX = (int)(paladin->GetPosition().x / (roomOuterSize * tileW));
                        currentGridY = (int)(paladin->GetPosition().y / (roomOuterSize * tileW));
                    }
                    MinimapRenderer::Draw(
                        levelManager.GetLevelMap(),
                        currentGridX,
                        currentGridY
                    );
                }
            } else if (renderState == GameState::HUB &&
                       !paladinSelectionMenu.IsOpen()) {
                DrawHubInteractionPrompt(FindNearestHubInteractable(
                    gameManager.GetLevelEntities(),
                    teamManager->GetActivePaladin()->GetPosition()
                ));
            }
            EndMode2D();
        } else if (renderState == GameState::GAME_OVER) {
            BeginMode2D(uiCamera);
            ClearBackground(BLACK);
            UIUtils::DrawCenteredText("PixeloidBold", "GAME OVER", { 400, 250 }, UIUtils::FontSize::TITLE, RED);
            UIUtils::DrawCenteredText("PixeloidSans", "Press R to Restart", { 400, 300 }, UIUtils::FontSize::BODY, LIGHTGRAY);
            EndMode2D();
        } else if (renderState == GameState::VICTORY) {
            BeginMode2D(uiCamera);
            ClearBackground(RAYWHITE);
            UIUtils::DrawCenteredText("PixeloidBold", "MISSION ACCOMPLISHED", { 400, 200 }, UIUtils::FontSize::TITLE, GOLD);
            UIUtils::DrawCenteredText("PixeloidSans", "Press R to return to Main Menu", { 400, 300 }, UIUtils::FontSize::BODY, DARKGRAY);
            EndMode2D();
        } else {
            mainMenu.Draw(GetScreenWidth(), GetScreenHeight());
        }

        if (state == GameState::PAUSE) {
            UIManager::DrawModalOverlay();
            BeginMode2D(modalCamera);
            pauseMenu.Draw(modalMousePosition);
            EndMode2D();
        } else if (state == GameState::SETTINGS) {
            UIManager::DrawModalOverlay();
            BeginMode2D(modalCamera);
            settingsMenu.Draw(modalMousePosition);
            EndMode2D();
        } else if (state == GameState::HUB &&
                   paladinSelectionMenu.IsOpen()) {
            UIManager::DrawModalOverlay();
            BeginMode2D(uiCamera);
            paladinSelectionMenu.Draw(
                uiMousePosition,
                *teamManager
            );
            EndMode2D();
        }

        adminPanel.Draw();

        EndDrawing();
    }

    delete teamManager;
    AssetManager::GetInstance().UnloadAll();
    CloseWindow();
    return 0;
}
