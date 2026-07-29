#include "raylib.h"
#include "raymath.h"

#include "Core/Constants.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/CameraManager.h"
#include "Core/Manager/DialogueManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/ParticleManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/WaveManager.h"
#include "Entities/NPC.h"
#include "Entities/Player/Hunk.h"
#include "Entities/Player/Keith.h"
#include "Entities/Player/Lance.h"
#include "UI/MainMenu.h"
#include "UI/PauseMenu.h"
#include "UI/UIManager.h"

#include <algorithm>

namespace {
    void ResetGame(
        TeamManager* teamManager,
        LevelManager* levelManager,
        WaveManager* waveManager
    ) {
        teamManager->GetActivePaladin()->SetPosition({256.0f, 256.0f});
        for (auto* paladin : teamManager->GetTeam()) {
            paladin->ResetStats();
        }
        GameManager::GetInstance().ClearProjectiles();
        levelManager->LoadLevel(
            "assets/map/level1_Tile Layer 1.csv",
            teamManager
        );
        waveManager->Reset();
        GameManager::GetInstance().SetState(GameState::GAMEPLAY);
    }

    void ResetDemoGame(
        TeamManager* teamManager,
        LevelManager* levelManager,
        WaveManager* waveManager
    ) {
        teamManager->GetActivePaladin()->SetPosition({256.0f, 256.0f});
        for (auto* paladin : teamManager->GetTeam()) {
            paladin->ResetStats();
        }
        GameManager::GetInstance().ClearProjectiles();
        levelManager->LoadLevel("assets/levels/demo-big.txt", teamManager);
        waveManager->Reset(10, 0, 5);
        GameManager::GetInstance().SetState(GameState::GAMEPLAY);
    }

    void ReturnToMainMenu(
        TeamManager* teamManager,
        LevelManager* levelManager
    ) {
        GameManager::GetInstance().ClearProjectiles();
        for (auto* paladin : teamManager->GetTeam()) {
            paladin->ResetStats();
        }
        teamManager->GetActivePaladin()->SetPosition({256.0f, 256.0f});
        levelManager->LoadLevel("assets/levels/hub.txt", teamManager);
        GameManager::GetInstance().SetState(GameState::MAIN_MENU);
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

    MainMenu mainMenu;
    mainMenu.Initialize();
    AssetManager::GetInstance().QueueCharacterAssets();

    PauseMenu pauseMenu;
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

    while (!WindowShouldClose() && !quitRequested) {
        float deltaTime = GetFrameTime();
        float scale = std::min(
            (float)GetScreenWidth() / Constants::GAME_WIDTH,
            (float)GetScreenHeight() / Constants::GAME_HEIGHT
        );

        Camera2D uiCamera = {};
        uiCamera.zoom = scale;
        uiCamera.offset = {
            (GetScreenWidth() - Constants::GAME_WIDTH * scale) / 2.0f,
            (GetScreenHeight() - Constants::GAME_HEIGHT * scale) / 2.0f
        };

        AudioManager::GetInstance().UpdateMusicStream();

        Vector2 mouseWorld = GetScreenToWorld2D(
            GetMousePosition(),
            CameraManager::GetInstance().GetCamera()
        );
        Vector2 uiMousePosition = GetScreenToWorld2D(
            GetMousePosition(),
            uiCamera
        );
        if (uiMousePosition.x < 0.0f ||
            uiMousePosition.x > Constants::GAME_WIDTH ||
            uiMousePosition.y < 0.0f ||
            uiMousePosition.y > Constants::GAME_HEIGHT) {
            uiMousePosition = {-1.0f, -1.0f};
        }

        GameState state = gameManager.GetState();

        if (state == GameState::MAIN_MENU &&
            mainMenu.IsReady() &&
            !systemInitialized) {
            gameManager.SetBulletImpactTexture(
                AssetManager::GetInstance().GetTexture("Lance_Impact")
            );

            Vector2 startPosition = {
                (float)Constants::GAME_WIDTH / 2.0f,
                (float)Constants::GAME_HEIGHT / 2.0f
            };
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

            uiManager.Initialize();
            uiManager.SetTeamManager(teamManager);

            levelManager.LoadLevel("assets/levels/hub.txt", teamManager);
            gameManager.SetLevelManager(&levelManager);
            systemInitialized = true;
        }

        if (systemInitialized) {
            teamManager->GetActivePaladin()->SetAimTarget(mouseWorld);

            bool keyboardPauseRequested =
                IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
            bool hudPauseRequested =
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
                if (mainMenu.ConsumeQuitRequest()) {
                    quitRequested = true;
                }
                if (gameManager.GetState() == GameState::SETTINGS) {
                    settingsReturnState = GameState::MAIN_MENU;
                }
                if (systemInitialized && IsKeyPressed(KEY_R)) {
                    ResetDemoGame(teamManager, &levelManager, &waveManager);
                }
                break;
            }
            case GameState::HUB:
                if (!systemInitialized) {
                    break;
                }
                if (DialogueManager::GetInstance().IsActive()) {
                    DialogueManager::GetInstance().Update(deltaTime);
                } else {
                    if (DialogueManager::GetInstance().IsMissionRequested()) {
                        DialogueManager::GetInstance().ClearMissionRequest();
                        ResetGame(teamManager, &levelManager, &waveManager);
                        break;
                    }

                    levelManager.UpdateLevel(deltaTime);
                    teamManager->Update(deltaTime);

                    if (IsKeyPressed(KEY_E)) {
                        for (auto* entity : gameManager.GetLevelEntities()) {
                            if (entity->GetObjectType() != GameObjectType::NPC) {
                                continue;
                            }
                            NPC* npc = static_cast<NPC*>(entity);
                            if (Vector2Distance(
                                    teamManager->GetActivePaladin()->GetPosition(),
                                    npc->GetPosition()
                                ) < 50.0f) {
                                DialogueManager::GetInstance().LoadDialogueTree(
                                    "assets/story/intro.txt"
                                );
                                DialogueManager::GetInstance().StartDialogue();
                                break;
                            }
                        }
                    }
                }
                gameManager.UpdateEffects(deltaTime);
                CameraManager::GetInstance().UpdateCamera(
                    teamManager->GetActivePaladin()->GetPosition(),
                    mouseWorld,
                    deltaTime,
                    levelManager.GetLevelBounds(),
                    false
                );
                break;
            case GameState::GAMEPLAY:
                if (!systemInitialized) {
                    break;
                }
                if (gameManager.GetHitstopTimer() > 0.0f) {
                    gameManager.UpdateHitstop(deltaTime);
                } else {
                    levelManager.UpdateLevel(deltaTime);
                    teamManager->Update(deltaTime);
                    gameManager.UpdateProjectiles(deltaTime, teamManager);
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
                switch (pauseMenu.Update(uiMousePosition)) {
                    case PauseMenuAction::Resume:
                        gameManager.ResumeGame();
                        break;
                    case PauseMenuAction::BackToMainMenu:
                        ReturnToMainMenu(teamManager, &levelManager);
                        break;
                    case PauseMenuAction::Quit:
                        quitRequested = true;
                        break;
                    case PauseMenuAction::None:
                        break;
                }
                break;
            case GameState::SETTINGS: {
                Rectangle backButton = {
                    GetScreenWidth() / 2.0f - 60.0f,
                    GetScreenHeight() / 2.0f + 70.0f,
                    120.0f,
                    40.0f
                };
                if (IsKeyPressed(KEY_ESCAPE) ||
                    (CheckCollisionPointRec(GetMousePosition(), backButton) &&
                     IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
                    AudioManager::GetInstance().PlayRandomClick();
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
                    ReturnToMainMenu(teamManager, &levelManager);
                }
                break;
        }

        state = gameManager.GetState();
        GameState renderState = gameManager.GetRenderState();

        BeginDrawing();
        ClearBackground(BLACK);

        if (renderState == GameState::HUB ||
            renderState == GameState::GAMEPLAY) {
            BeginMode2D(CameraManager::GetInstance().GetCamera());
            ClearBackground(
                renderState == GameState::HUB ? DARKGREEN : DARKGRAY
            );
            levelManager.DrawLevel();
            gameManager.DrawEffects(true);
            teamManager->Draw();
            if (renderState == GameState::GAMEPLAY) {
                gameManager.DrawProjectiles();
            }
            ParticleManager::GetInstance().Draw();
            gameManager.DrawEffects(false);
            EndMode2D();

            BeginMode2D(uiCamera);
            uiManager.DrawHUD(
                Constants::GAME_WIDTH,
                Constants::GAME_HEIGHT,
                uiMousePosition
            );
            if (renderState == GameState::HUB) {
                DialogueManager::GetInstance().Draw(
                    Constants::GAME_WIDTH,
                    Constants::GAME_HEIGHT
                );
            } else if (state == GameState::GAMEPLAY) {
                waveManager.DrawHUD();
            }
            EndMode2D();
        } else if (renderState == GameState::GAME_OVER) {
            BeginMode2D(uiCamera);
            ClearBackground(BLACK);
            DrawText("GAME OVER", 180, 220, 30, RED);
            DrawText("Press R to Restart", 160, 280, 20, LIGHTGRAY);
            EndMode2D();
        } else if (renderState == GameState::VICTORY) {
            BeginMode2D(uiCamera);
            ClearBackground(RAYWHITE);
            DrawText("MISSION ACCOMPLISHED", 90, 200, 40, GOLD);
            DrawText(
                "Press R to return to Main Menu",
                150,
                300,
                20,
                DARKGRAY
            );
            EndMode2D();
        } else {
            mainMenu.Draw(GetScreenWidth(), GetScreenHeight());
        }

        if (state == GameState::PAUSE) {
            UIManager::DrawModalOverlay();
            BeginMode2D(uiCamera);
            pauseMenu.Draw(uiMousePosition);
            EndMode2D();
        } else if (state == GameState::SETTINGS) {
            UIManager::DrawModalOverlay();
            Rectangle popupBounds = {
                GetScreenWidth() / 2.0f - 200.0f,
                GetScreenHeight() / 2.0f - 150.0f,
                400.0f,
                300.0f
            };
            UIManager::DrawPopupFrame(popupBounds, "SETTINGS");

            Rectangle backButton = {
                GetScreenWidth() / 2.0f - 60.0f,
                GetScreenHeight() / 2.0f + 70.0f,
                120.0f,
                40.0f
            };
            bool hovered = CheckCollisionPointRec(
                GetMousePosition(),
                backButton
            );
            DrawRectangleRounded(
                backButton,
                0.2f,
                10,
                hovered ? LIGHTGRAY : DARKGRAY
            );
            DrawRectangleRoundedLinesEx(
                backButton,
                0.2f,
                10,
                2.0f,
                BLACK
            );
            int textWidth = MeasureText("BACK", 20);
            DrawText(
                "BACK",
                (int)(backButton.x + (backButton.width - textWidth) / 2.0f),
                (int)(backButton.y + 10.0f),
                20,
                hovered ? BLACK : WHITE
            );
        }

        EndDrawing();
    }

    delete teamManager;
    AssetManager::GetInstance().UnloadAll();
    CloseWindow();
    return 0;
}
