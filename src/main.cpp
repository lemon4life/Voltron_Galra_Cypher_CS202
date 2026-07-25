#include "raylib.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Lance.h"
#include "Entities/Player/Keith.h"
#include "Entities/Player/Hunk.h"
#include "Entities/Player/PlaceholderPaladin.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/LevelManager.h"
#include "Combat/MeleeAttackStrategy.h"
#include "Combat/RangedAttackStrategy.h"
#include "UI/UIManager.h"
#include "Core/Manager/WaveManager.h"
#include "Core/Manager/DialogueManager.h"
#include "Entities/NPC.h"
#include "GUI/MainMenu.h"
#include "GUI/PauseMenu.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr int INITIAL_WINDOW_WIDTH = 1366;
    constexpr int INITIAL_WINDOW_HEIGHT = 1024;
    constexpr int GAME_WIDTH = 683;
    constexpr int GAME_HEIGHT = 512;
    constexpr int BASE_FPS = 120;
}

void ResetGame(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager) {
    teamManager->GetActivePaladin()->SetPosition({ 256.0f, 256.0f });
    for(auto p : teamManager->GetTeam()) p->ResetStats();
    GameManager::GetInstance().ClearProjectiles();
    levelManager->LoadLevel("assets/map/level1_Tile Layer 1.csv", teamManager);
    waveManager->Reset();
    GameManager::GetInstance().SetState(GameState::PLAYING);
}

void ResetDemoGame(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager) {
    teamManager->GetActivePaladin()->SetPosition({ 256.0f, 256.0f });
    for(auto p : teamManager->GetTeam()) p->ResetStats();
    GameManager::GetInstance().ClearProjectiles();
    levelManager->LoadLevel("assets/levels/demo-big.txt", teamManager);
    waveManager->Reset(10, 0, 5);
    GameManager::GetInstance().SetState(GameState::PLAYING);
}

void ReturnToMainMenu(TeamManager* teamManager, LevelManager* levelManager) {
    GameManager::GetInstance().ClearProjectiles();
    for (auto* paladin : teamManager->GetTeam()) {
        paladin->ResetStats();
    }
    teamManager->GetActivePaladin()->SetPosition({256.0f, 256.0f});
    levelManager->LoadLevel("assets/levels/hub.txt", teamManager);
    GameManager::GetInstance().SetState(GameState::MENU);
}

int main() {
    // Initialize Window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(
        INITIAL_WINDOW_WIDTH,
        INITIAL_WINDOW_HEIGHT,
        "Voltron: Mission Galra Cypher"
    );

    // Initialize AudioManager Singleton (Initializes Audio Device)
    AudioManager::GetInstance();
    AudioManager::GetInstance().Initialize();
    MainMenu mainMenu;
    PauseMenu pauseMenu;
    bool quitRequested = false;

    // Initialize Dialogue Assets
    DialogueManager::GetInstance().InitializeAssets();

    // Start background music
    AudioManager::GetInstance().PlayMusicTrack("bgm");

    // Initialize AssetManager and load character textures
    AssetManager::GetInstance().LoadCharacterAssets();
    GameManager::GetInstance().SetBulletImpactTexture(AssetManager::GetInstance().GetTexture("Lance_Impact"));

    // Initialize TeamManager and Paladins
    Vector2 startPos = { (float)GAME_WIDTH / 2.0f, (float)GAME_HEIGHT / 2.0f };
    TeamManager* teamManager = new TeamManager();
    
    Lance* lance = new Lance(startPos, AssetManager::GetInstance().GetLanceSprites());
    Keith* keith = new Keith(startPos, AssetManager::GetInstance().GetKeithSprites());
    Hunk* hunk = new Hunk(startPos, AssetManager::GetInstance().GetHunkSprites());
    
    teamManager->AddMember(lance);
    teamManager->AddMember(keith);
    teamManager->AddMember(hunk);

    // Initialize UI Manager
    UIManager uiManager;
    uiManager.Initialize();
    uiManager.SetTeamManager(teamManager);

    // Setup LevelManager
    LevelManager levelManager;
    levelManager.LoadLevel("assets/levels/hub.txt", teamManager);
    GameManager::GetInstance().SetLevelManager(&levelManager);

    // Initialize WaveManager
    WaveManager waveManager;

    // Initialize Camera
    Camera2D camera = { 0 };
    camera.target = { 0.0f, 0.0f };
    camera.offset = { std::round(INITIAL_WINDOW_WIDTH / 2.0f), std::round(INITIAL_WINDOW_HEIGHT / 2.0f) };
    camera.rotation = 0.0f;
    camera.zoom = 2.0f;

    GameManager::GetInstance().UpdateTargetFPS(BASE_FPS);

    // Main Game Loop
    while (!WindowShouldClose() && !quitRequested) {
        float deltaTime = GetFrameTime();
        
        // Dynamic camera scaling based on window size
        float scale = std::min((float)GetScreenWidth() / GAME_WIDTH, (float)GetScreenHeight() / GAME_HEIGHT);
        camera.offset = { (float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f };
        
        float hitstopZoom = (GameManager::GetInstance().GetHitstopTimer() > 0.0f) ? 1.1f : 1.0f;
        camera.zoom = Lerp(camera.zoom, scale * hitstopZoom, 15.0f * deltaTime);

        Camera2D uiCamera = { 0 };
        uiCamera.zoom = scale;
        uiCamera.offset = { 
            (GetScreenWidth() - (GAME_WIDTH * scale)) / 2.0f, 
            (GetScreenHeight() - (GAME_HEIGHT * scale)) / 2.0f 
        };

        // Update music stream continuously regardless of game state
        AudioManager::GetInstance().UpdateMusicStream();
        
        // Pass mouse coordinates to player for aiming
        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
        teamManager->GetActivePaladin()->SetAimTarget(mouseWorld);
        
        // UI mouse calculation relative to the scaled virtual resolution
        Vector2 uiMousePosition = GetScreenToWorld2D(GetMousePosition(), uiCamera);
        if (uiMousePosition.x < 0 || uiMousePosition.x > GAME_WIDTH || 
            uiMousePosition.y < 0 || uiMousePosition.y > GAME_HEIGHT) {
            uiMousePosition = { -1.0f, -1.0f };
        }



        GameState state = GameManager::GetInstance().GetState();
        
        switch (state) {
            case GameState::MENU:
                switch (mainMenu.Update(uiMousePosition)) {
                    case MainMenuAction::Play:
                        GameManager::GetInstance().SetState(GameState::HUB);
                        break;
                    case MainMenuAction::Quit:
                        quitRequested = true;
                        break;
                    case MainMenuAction::None:
                        break;
                }
                if (IsKeyPressed(KEY_R)) {
                    ResetDemoGame(teamManager, &levelManager, &waveManager);
                }
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
                camera.target.x = Lerp(camera.target.x, teamManager->GetActivePaladin()->GetPosition().x, 20.0f * deltaTime);
                camera.target.y = Lerp(camera.target.y, teamManager->GetActivePaladin()->GetPosition().y, 20.0f * deltaTime);
                break;
            case GameState::PLAYING:
                if (GameManager::GetInstance().GetHitstopTimer() > 0.0f) {
                    GameManager::GetInstance().UpdateHitstop(deltaTime);
                } else {
                    levelManager.UpdateLevel(deltaTime);
                    teamManager->Update(deltaTime);
                    GameManager::GetInstance().UpdateProjectiles(deltaTime, teamManager);
                    waveManager.Update(deltaTime, teamManager, &levelManager);
                }
                camera.target.x = Lerp(camera.target.x, teamManager->GetActivePaladin()->GetPosition().x, 20.0f * deltaTime);
                camera.target.y = Lerp(camera.target.y, teamManager->GetActivePaladin()->GetPosition().y, 20.0f * deltaTime);
                
                if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                    GameManager::GetInstance().SetState(GameState::PAUSED);
                }

                break;
            case GameState::PAUSED:
                if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                    GameManager::GetInstance().SetState(GameState::PLAYING);
                } else {
                    switch (pauseMenu.Update(uiMousePosition)) {
                        case PauseMenuAction::Resume:
                            GameManager::GetInstance().SetState(GameState::PLAYING);
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
                }
                break;
            case GameState::GAMEOVER:
                if (IsKeyPressed(KEY_R)) {
                    ResetGame(teamManager, &levelManager, &waveManager);
                }
                break;
            case GameState::VICTORY:
                if (IsKeyPressed(KEY_R)) {
                    GameManager::GetInstance().SetState(GameState::MENU);
                    ResetGame(teamManager, &levelManager, &waveManager);
                }
                break;
        }

        state = GameManager::GetInstance().GetState();

        // --- Draw ---
        BeginDrawing();
            ClearBackground(BLACK);

            if (state == GameState::MENU) {
                BeginMode2D(uiCamera);
                mainMenu.Draw(uiMousePosition);
                EndMode2D();
            } else if (state == GameState::GAMEOVER) {
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
            } else {
                BeginMode2D(camera);
                
                if (state == GameState::HUB) {
                    ClearBackground(DARKGREEN);
                    levelManager.DrawLevel();
                    GameManager::GetInstance().UpdateEffects(deltaTime);
                    GameManager::GetInstance().DrawEffects(true); // background
                    teamManager->Draw();
                    GameManager::GetInstance().DrawEffects(false); // foreground
                } else if (state == GameState::PLAYING || state == GameState::PAUSED) {
                    ClearBackground(DARKGRAY);
                    levelManager.DrawLevel();
                    GameManager::GetInstance().UpdateEffects(deltaTime);
                    GameManager::GetInstance().DrawEffects(true); // background
                    teamManager->Draw();
                    GameManager::GetInstance().DrawProjectiles();
                    GameManager::GetInstance().DrawEffects(false); // foreground
                }
                
                EndMode2D();
                
                // Draw HUD outside of camera
                BeginMode2D(uiCamera);
                if (state == GameState::HUB) {
                    uiManager.DrawHUD(GAME_WIDTH, GAME_HEIGHT, uiMousePosition);
                    DialogueManager::GetInstance().Draw(GAME_WIDTH, GAME_HEIGHT);
                } else if (state == GameState::PLAYING) {
                    uiManager.DrawHUD(GAME_WIDTH, GAME_HEIGHT, uiMousePosition);
                    waveManager.DrawHUD();
                } else if (state == GameState::PAUSED) {
                    uiManager.DrawHUD(GAME_WIDTH, GAME_HEIGHT, uiMousePosition);
                    pauseMenu.Draw(uiMousePosition);
                }
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
