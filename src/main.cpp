#include "raylib.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Lance.h"
#include "Entities/Player/Keith.h"
#include "Entities/Player/Hunk.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/LevelManager.h"
#include "Combat/MeleeAttackStrategy.h"
#include "Combat/RangedAttackStrategy.h"
#include "UI/UIManager.h"
#include "UI/MainMenu.h"
#include "Core/Manager/WaveManager.h"
#include "Core/Manager/CameraManager.h"
#include "Core/Manager/DialogueManager.h"
#include "Entities/NPC.h"
#include "raymath.h"
#include "Core/Manager/ParticleManager.h"

#include <algorithm>
#include <cmath>
#include "Core/Constants.h"



void ResetGame(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager) {
    teamManager->GetActivePaladin()->SetPosition({ 256.0f, 256.0f });
    for(auto p : teamManager->GetTeam()) p->ResetStats();
    GameManager::GetInstance().ClearProjectiles();
    levelManager->LoadLevel("assets/map/level1_Tile Layer 1.csv", teamManager);
    waveManager->Reset();
    GameManager::GetInstance().SetState(GameState::GAMEPLAY);
}

void ResetDemoGame(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager) {
    teamManager->GetActivePaladin()->SetPosition({ 256.0f, 256.0f });
    for(auto p : teamManager->GetTeam()) p->ResetStats();
    GameManager::GetInstance().ClearProjectiles();
    levelManager->LoadLevel("assets/levels/demo-big.txt", teamManager);
    waveManager->Reset(10, 0, 5);
    GameManager::GetInstance().SetState(GameState::GAMEPLAY);
}

int main() {
    // Initialize Window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(
        Constants::SCREEN_WIDTH,
        Constants::SCREEN_HEIGHT,
        Constants::GAME_TITLE
    );

    // Initialize AudioManager Singleton (Initializes Audio Device)
    AudioManager::GetInstance();
    AudioManager::GetInstance().Initialize();
    // Initialize ParticleManager — loads the silhouette shader (must be after InitWindow)
    ParticleManager::GetInstance().Initialize();

    // Initialize Dialogue Assets
    DialogueManager::GetInstance().InitializeAssets();

    // Start background music
    AudioManager::GetInstance().PlayMusicTrack("bgm");

    // Initialize Main Menu so background loads instantly
    MainMenu mainMenu;
    mainMenu.Initialize();

    // Queue character assets for chunked loading during the Loading Screen
    AssetManager::GetInstance().QueueCharacterAssets();

    // Defer heavy initializations until loading completes
    TeamManager* teamManager = nullptr;
    UIManager uiManager;
    LevelManager levelManager;
    WaveManager waveManager;
    bool systemInitialized = false;
    
    // Initialize Camera (setup defaults)
    CameraManager::GetInstance().Initialize();

    GameManager::GetInstance().UpdateTargetFPS(Constants::TARGET_FPS);

// Main Game Loop
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        float scale = std::min((float)GetScreenWidth() / Constants::GAME_WIDTH, (float)GetScreenHeight() / Constants::GAME_HEIGHT);

        Camera2D uiCamera = { 0 };
        uiCamera.zoom = scale;
        uiCamera.offset = { 
            (GetScreenWidth() - (Constants::GAME_WIDTH * scale)) / 2.0f, 
            (GetScreenHeight() - (Constants::GAME_HEIGHT * scale)) / 2.0f 
        };

        // Update music stream continuously regardless of game state
        AudioManager::GetInstance().UpdateMusicStream();
        
        // Pass mouse coordinates to player for aiming
        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), CameraManager::GetInstance().GetCamera());
        if (systemInitialized) teamManager->GetActivePaladin()->SetAimTarget(mouseWorld);
        
        // UI mouse calculation relative to the scaled virtual resolution
        Vector2 uiMousePosition = GetScreenToWorld2D(GetMousePosition(), uiCamera);
        if (uiMousePosition.x < 0 || uiMousePosition.x > Constants::GAME_WIDTH || 
            uiMousePosition.y < 0 || uiMousePosition.y > Constants::GAME_HEIGHT) {
            uiMousePosition = { -1.0f, -1.0f };
        }



        GameState state = GameManager::GetInstance().GetState();
        static GameState previousState = GameState::MAIN_MENU;
        
        // Check if loading is fully done and game is ready to initialize
        if (state == GameState::MAIN_MENU && mainMenu.IsReady() && !systemInitialized) {
            GameManager::GetInstance().SetBulletImpactTexture(AssetManager::GetInstance().GetTexture("Lance_Impact"));

            Vector2 startPos = { (float)Constants::GAME_WIDTH / 2.0f, (float)Constants::GAME_HEIGHT / 2.0f };
            teamManager = new TeamManager();
            
            Lance* lance = new Lance(startPos, AssetManager::GetInstance().GetLanceSprites());
            Keith* keith = new Keith(startPos, AssetManager::GetInstance().GetKeithSprites());
            Hunk* hunk = new Hunk(startPos, AssetManager::GetInstance().GetHunkSprites());
            
            teamManager->AddMember(lance);
            teamManager->AddMember(keith);
            teamManager->AddMember(hunk);

            uiManager.Initialize();
            uiManager.SetTeamManager(teamManager);

            levelManager.LoadLevel("assets/levels/hub.txt", teamManager);
            GameManager::GetInstance().SetLevelManager(&levelManager);
            
            systemInitialized = true;
        }
        
        switch (state) {
            case GameState::MAIN_MENU:
                mainMenu.Update(deltaTime);
                break;
            case GameState::HUB:
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
                        for (auto* entity : GameManager::GetInstance().GetLevelEntities()) {
                            if (entity->GetObjectType() == GameObjectType::NPC) {
                                NPC* npc = static_cast<NPC*>(entity);
                                if (Vector2Distance(teamManager->GetActivePaladin()->GetPosition(), npc->GetPosition()) < 50.0f) {
                                    DialogueManager::GetInstance().LoadDialogueTree("assets/story/intro.txt");
                                    DialogueManager::GetInstance().StartDialogue();
                                    break;
                                }
                            }
                        }
                    }
                }
                CameraManager::GetInstance().UpdateCamera(
                    teamManager->GetActivePaladin()->GetPosition(),
                    mouseWorld,
                    deltaTime,
                    levelManager.GetLevelBounds(),
                    false
                );
                break;
            case GameState::GAMEPLAY:
                if (GameManager::GetInstance().GetHitstopTimer() > 0.0f) {
                    GameManager::GetInstance().UpdateHitstop(deltaTime);
                } else {
                    levelManager.UpdateLevel(deltaTime);
                    teamManager->Update(deltaTime);
                    GameManager::GetInstance().UpdateProjectiles(deltaTime, teamManager);
                    waveManager.Update(deltaTime, teamManager, &levelManager);
                }
                ParticleManager::GetInstance().Update(deltaTime);
                CameraManager::GetInstance().UpdateCamera(
                    teamManager->GetActivePaladin()->GetPosition(),
                    mouseWorld,
                    deltaTime,
                    levelManager.GetLevelBounds(),
                    (GameManager::GetInstance().GetHitstopTimer() > 0.0f)
                );
                
                if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                    previousState = GameState::GAMEPLAY;
                    GameManager::GetInstance().SetState(GameState::PAUSE);
                }
                break;
            case GameState::PAUSE:
                if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                    GameManager::GetInstance().SetState(GameState::GAMEPLAY);
                }
                break;
            case GameState::SETTINGS:
                if (IsKeyPressed(KEY_ESCAPE)) {
                    GameManager::GetInstance().SetState(previousState);
                }
                break;
            case GameState::GAME_OVER:
                if (IsKeyPressed(KEY_R)) {
                    ResetGame(teamManager, &levelManager, &waveManager);
                }
                break;
            case GameState::VICTORY:
                if (IsKeyPressed(KEY_R)) {
                    GameManager::GetInstance().SetState(GameState::MAIN_MENU);
                    ResetGame(teamManager, &levelManager, &waveManager);
                }
                break;
        }

        // --- Draw ---
        BeginDrawing();
            ClearBackground(BLACK);

            if (state == GameState::HUB || state == GameState::GAMEPLAY || state == GameState::PAUSE || state == GameState::SETTINGS) {
                // Determine if we should draw gameplay in background
                bool drawGameplay = true;
                if (state == GameState::SETTINGS && previousState == GameState::MAIN_MENU) {
                    drawGameplay = false;
                }

                if (drawGameplay) {
                    BeginMode2D(CameraManager::GetInstance().GetCamera());
                        if (state == GameState::HUB) {
                            ClearBackground(DARKGREEN);
                            levelManager.DrawLevel();
                            GameManager::GetInstance().UpdateEffects(deltaTime);
                            GameManager::GetInstance().DrawEffects(true);
                            teamManager->Draw();
                            ParticleManager::GetInstance().Draw();
                            GameManager::GetInstance().DrawEffects(false);
                        } else {
                            ClearBackground(DARKGRAY);
                            levelManager.DrawLevel();
                            GameManager::GetInstance().UpdateEffects(deltaTime);
                            GameManager::GetInstance().DrawEffects(true);
                            teamManager->Draw();
                            GameManager::GetInstance().DrawProjectiles();
                            ParticleManager::GetInstance().Draw();
                            GameManager::GetInstance().DrawEffects(false);
                        }
                    EndMode2D();

                    BeginMode2D(uiCamera);
                        if (state == GameState::HUB) {
                            uiManager.DrawHUD(Constants::GAME_WIDTH, Constants::GAME_HEIGHT, uiMousePosition);
                            DialogueManager::GetInstance().Draw(Constants::GAME_WIDTH, Constants::GAME_HEIGHT);
                        } else {
                            uiManager.DrawHUD(Constants::GAME_WIDTH, Constants::GAME_HEIGHT, uiMousePosition);
                            if (state == GameState::GAMEPLAY) {
                                waveManager.DrawHUD();
                            }
                        }
                    EndMode2D();
                }
            }

            // Screen-space UI for overlays and menus
            int sw = GetScreenWidth();
            int sh = GetScreenHeight();
            Vector2 mousePos = GetMousePosition();

            // Helper for simple buttons
            auto DrawButton = [&](Rectangle bounds, const char* text) -> bool {
                bool hovered = CheckCollisionPointRec(mousePos, bounds);
                DrawRectangleRounded(bounds, 0.2f, 10, hovered ? LIGHTGRAY : DARKGRAY);
                DrawRectangleRoundedLinesEx(bounds, 0.2f, 10, 2.0f, BLACK);
                int tw = MeasureText(text, 20);
                DrawText(text, bounds.x + (bounds.width - tw)/2, bounds.y + (bounds.height - 20)/2, 20, hovered ? BLACK : WHITE);
                return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            };

            if (state == GameState::MAIN_MENU || (state == GameState::SETTINGS && previousState == GameState::MAIN_MENU)) {
                mainMenu.Draw(sw, sh);
            } 
            
            if (state == GameState::PAUSE || (state == GameState::SETTINGS && previousState == GameState::PAUSE)) {
                if (state == GameState::PAUSE) {
                    UIManager::DrawModalOverlay();
                    Rectangle popupBounds = { sw / 2.0f - 250, sh / 2.0f - 100, 500, 200 };
                    UIManager::DrawPopupFrame(popupBounds, "PAUSED");

                    float btnW = 120;
                    float btnH = 40;
                    float startX = popupBounds.x + 40;
                    float btnY = popupBounds.y + 100;

                    if (DrawButton({startX, btnY, btnW, btnH}, "HOME")) {
                        AudioManager::GetInstance().PlayRandomClick();
                        GameManager::GetInstance().SetState(GameState::MAIN_MENU);
                    }
                    if (DrawButton({startX + 150, btnY, btnW, btnH}, "CONTINUE")) {
                        AudioManager::GetInstance().PlayRandomClick();
                        GameManager::GetInstance().SetState(GameState::GAMEPLAY);
                    }
                    if (DrawButton({startX + 300, btnY, btnW, btnH}, "SETTINGS")) {
                        AudioManager::GetInstance().PlayRandomClick();
                        previousState = GameState::PAUSE;
                        GameManager::GetInstance().SetState(GameState::SETTINGS);
                    }
                }
            }

            if (state == GameState::SETTINGS) {
                UIManager::DrawModalOverlay();
                Rectangle popupBounds = { sw / 2.0f - 200, sh / 2.0f - 150, 400, 300 };
                UIManager::DrawPopupFrame(popupBounds, "SETTINGS");
                
                if (DrawButton({sw / 2.0f - 60, sh / 2.0f + 70, 120, 40}, "BACK")) {
                    AudioManager::GetInstance().PlayRandomClick();
                    GameManager::GetInstance().SetState(previousState);
                }
            }

            if (state == GameState::GAME_OVER) {
                BeginMode2D(uiCamera);
                ClearBackground(BLACK);
                DrawText("GAME OVER", 180, 220, 30, RED);
                DrawText("Press R to Restart", 160, 280, 20, LIGHTGRAY);
                EndMode2D();
            } else if (state == GameState::VICTORY) {
                BeginMode2D(uiCamera);
                ClearBackground(RAYWHITE);
                DrawText("MISSION ACCOMPLISHED", 90, 200, 40, GOLD);
                DrawText("Press R to return to Main Menu", 150, 300, 20, DARKGRAY);
                EndMode2D();
            }

        EndDrawing();
    }

    // De-Initialization
    delete teamManager;
    AssetManager::GetInstance().UnloadAll();
    CloseWindow();

    return 0;
}
