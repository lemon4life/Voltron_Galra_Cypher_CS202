#include "raylib.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Lance.h"
#include "Entities/Player/Keith.h"
#include "Entities/Player/Hunk.h"
#include "Entities/Player/PlaceholderPaladin.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/LevelManager.h"
#include "Combat/MeleeAttackStrategy.h"
#include "Combat/RangedAttackStrategy.h"
#include "UI/UIManager.h"
#include "Core/Manager/WaveManager.h"
#include "Core/Manager/DialogueManager.h"
#include "Entities/NPC.h"
#include "raymath.h"

// Init Window config
// Window and Game Resolutions (4:3 aspect ratio landscape approx)
const int WINDOW_WIDTH = 1366;
const int WINDOW_HEIGHT = 1024;

const int GAME_WIDTH = 683;
const int GAME_HEIGHT = 512;
const int BASE_FPS = 120;

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
    waveManager->Reset(150);
    GameManager::GetInstance().SetState(GameState::PLAYING);
}


int main() {
    // Initialize Window
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Voltron: Mission Galra Cypher");

    // Initialize AudioManager Singleton (Initializes Audio Device)
    AudioManager::GetInstance();

    // Load Audio
    AudioManager::GetInstance().Initialize();

    // Initialize Dialogue Assets
    DialogueManager::GetInstance().InitializeAssets();

    // Start background music
    AudioManager::GetInstance().PlayMusicTrack("bgm");

    CharacterSprites lanceSprites;
    lanceSprites.idle = LoadTexture("assets/sprites/Lance/Idle_Sheet.png");
    lanceSprites.run = LoadTexture("assets/sprites/Lance/Run_Sheet.png");
    lanceSprites.weapon = LoadTexture("assets/sprites/Lance/Weapon_Static.png");
    lanceSprites.muzzleFlash = LoadTexture("assets/sprites/Lance/Muzzle_Flash.png");
    lanceSprites.bullet = LoadTexture("assets/sprites/Lance/Bullet.png");
    lanceSprites.impact = LoadTexture("assets/sprites/Lance/Bullet_Impact.png");
    lanceSprites.dashFront = LoadTexture("assets/sprites/Lance/Dash_front.png");
    lanceSprites.dashBack = LoadTexture("assets/sprites/Lance/Dash_back.png");
    
    SetTextureFilter(lanceSprites.weapon, TEXTURE_FILTER_POINT);
    SetTextureFilter(lanceSprites.muzzleFlash, TEXTURE_FILTER_POINT);
    SetTextureFilter(lanceSprites.bullet, TEXTURE_FILTER_POINT);
    SetTextureFilter(lanceSprites.impact, TEXTURE_FILTER_POINT);
    GameManager::GetInstance().SetBulletImpactTexture(lanceSprites.impact);

    CharacterSprites keithSprites;
    keithSprites.idle = LoadTexture("assets/sprites/Keith/Idle_Sheet.png");
    keithSprites.run = LoadTexture("assets/sprites/Keith/Run_Sheet.png");
    keithSprites.weapon = LoadTexture("assets/sprites/Keith/Weapon_Static.png");
    SetTextureFilter(keithSprites.weapon, TEXTURE_FILTER_POINT);
    keithSprites.attack1 = LoadTexture("assets/sprites/Keith/Attack_1.png");
    keithSprites.attack2 = LoadTexture("assets/sprites/Keith/Attack_2.png");
    
    SetTextureFilter(keithSprites.attack1, TEXTURE_FILTER_POINT);
    SetTextureFilter(keithSprites.attack2, TEXTURE_FILTER_POINT);

    // Initialize TeamManager and Paladins
    Vector2 startPos = { (float)GAME_WIDTH / 2.0f, (float)GAME_HEIGHT / 2.0f };
    TeamManager* teamManager = new TeamManager();
    Lance* lance = new Lance(startPos, lanceSprites);
    Keith* keith = new Keith(startPos, keithSprites);
    CharacterSprites hunkSprites;
    hunkSprites.idle = LoadTexture("assets/sprites/Hunk/Idle_Sheet.png");
    hunkSprites.run = LoadTexture("assets/sprites/Hunk/Run_Sheet.png");
    hunkSprites.weapon = LoadTexture("assets/sprites/Hunk/Weapon_Static.png");
    hunkSprites.muzzleFlash = LoadTexture("assets/sprites/Hunk/Muzzle.png");
    hunkSprites.bullet = LoadTexture("assets/sprites/Hunk/Beam.png");
    hunkSprites.impact = LoadTexture("assets/sprites/Hunk/Beam_Impact.png");
    
    SetTextureFilter(hunkSprites.weapon, TEXTURE_FILTER_POINT);
    SetTextureFilter(hunkSprites.muzzleFlash, TEXTURE_FILTER_POINT);
    SetTextureFilter(hunkSprites.bullet, TEXTURE_FILTER_POINT);
    SetTextureFilter(hunkSprites.impact, TEXTURE_FILTER_POINT);
    
    Hunk* hunk = new Hunk(startPos, hunkSprites);
    
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
    camera.offset = { std::round(WINDOW_WIDTH / 2.0f), std::round(WINDOW_HEIGHT / 2.0f) };
    camera.rotation = 0.0f;
    camera.zoom = 2.0f;
    
    Camera2D uiCamera = { 0 };
    uiCamera.zoom = 2.0f;

    GameManager::GetInstance().UpdateTargetFPS(BASE_FPS);

    // Main Game Loop
    while (!WindowShouldClose()) {
        // --- Update ---
        float deltaTime = GetFrameTime();

        // Update music stream continuously regardless of game state
        AudioManager::GetInstance().UpdateMusicStream();
        
        // Pass mouse coordinates to player for aiming
        Vector2 mouseScreen = GetMousePosition();
        Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);
        teamManager->GetActivePaladin()->SetAimTarget(mouseWorld);

        // Mock EX Generation Input
        if (IsKeyPressed(KEY_SPACE)) {
            teamManager->GetActivePaladin()->OnHitEnemy(50);
        }

        GameState state = GameManager::GetInstance().GetState();
        
        switch (state) {
            case GameState::MENU:
                if (IsKeyPressed(KEY_ENTER)) {
                    AudioManager::GetInstance().PlayRandomClick();
                    GameManager::GetInstance().SetState(GameState::HUB);
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
                    camera.target = { teamManager->GetActivePaladin()->GetPosition().x, teamManager->GetActivePaladin()->GetPosition().y };
                    
                    if (IsKeyPressed(KEY_E)) {
                        for (auto* entity : GameManager::GetInstance().GetLevelEntities()) {
                            if (NPC* npc = dynamic_cast<NPC*>(entity)) {
                                if (Vector2Distance(teamManager->GetActivePaladin()->GetPosition(), npc->GetPosition()) < 50.0f) {
                                    DialogueManager::GetInstance().LoadDialogueTree("assets/story/intro.txt");
                                    DialogueManager::GetInstance().StartDialogue();
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
            case GameState::PLAYING:
                levelManager.UpdateLevel(deltaTime);
                teamManager->Update(deltaTime);
                GameManager::GetInstance().UpdateProjectiles(deltaTime, teamManager);
                waveManager.Update(deltaTime, teamManager, &levelManager);
                camera.target = { teamManager->GetActivePaladin()->GetPosition().x, teamManager->GetActivePaladin()->GetPosition().y };
                
                if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                    GameManager::GetInstance().SetState(GameState::PAUSED);
                }
                if (teamManager->GetActivePaladin()->GetHealth() <= 0) {
                    GameManager::GetInstance().SetState(GameState::GAMEOVER);
                }
                break;
            case GameState::PAUSED:
                if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                    GameManager::GetInstance().SetState(GameState::PLAYING);
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

        // --- Draw ---
        BeginDrawing();
            if (state == GameState::MENU) {
                ClearBackground(DARKGRAY);
                BeginMode2D(uiCamera);
                DrawText("Voltron: Mission Galra Cypher", 70, 200, 24, WHITE);
                DrawText("Press ENTER to Start", 140, 300, 20, LIGHTGRAY);
                DrawText("Press R to Enter Demo Map", 120, 335, 20, LIGHTGRAY);
                EndMode2D();
            } else if (state == GameState::GAMEOVER) {
                ClearBackground(BLACK);
                BeginMode2D(uiCamera);
                DrawText("GAME OVER", 180, 220, 30, RED);
                DrawText("Press R to Restart", 160, 280, 20, LIGHTGRAY);
                EndMode2D();
            } else if (state == GameState::VICTORY) {
                ClearBackground(RAYWHITE);
                BeginMode2D(uiCamera);
                DrawText("MISSION ACCOMPLISHED", 90, 200, 40, GOLD);
                DrawText("Press R to return to Main Menu", 150, 300, 20, DARKGRAY);
                EndMode2D();
            } else {
                BeginMode2D(camera);
                
                if (state == GameState::HUB) {
                    ClearBackground(DARKGREEN);
                    levelManager.DrawLevel();
                    teamManager->Draw();
                } else if (state == GameState::PLAYING) {
                    ClearBackground(DARKGRAY);
                    levelManager.DrawLevel();
                    teamManager->Draw();
                    GameManager::GetInstance().DrawProjectiles();
                    GameManager::GetInstance().UpdateAndDrawEffects(deltaTime);
                } else if (state == GameState::PAUSED) {
                    ClearBackground(DARKGRAY);
                    levelManager.DrawLevel();
                    teamManager->Draw();
                    GameManager::GetInstance().DrawProjectiles();
                    GameManager::GetInstance().UpdateAndDrawEffects(deltaTime);
                }
                
                EndMode2D();
                
                // Draw HUD outside of camera but with uiCamera scale
                BeginMode2D(uiCamera);
                if (state == GameState::HUB) {
                    uiManager.DrawHUD(GAME_WIDTH, GAME_HEIGHT);
                } else if (state == GameState::PLAYING) {
                    uiManager.DrawHUD(GAME_WIDTH, GAME_HEIGHT);
                    waveManager.DrawHUD();
                } else if (state == GameState::PAUSED) {
                    uiManager.DrawHUD(GAME_WIDTH, GAME_HEIGHT);
                    DrawRectangle(0, 0, GAME_WIDTH, GAME_HEIGHT, {0, 0, 0, 150});
                    DrawText("PAUSED", GAME_WIDTH / 2 - MeasureText("PAUSED", 40) / 2, GAME_HEIGHT / 2 - 20, 40, RAYWHITE);
                }
                EndMode2D();
                
                // Draw high-res UI elements on top of the scaled game
                if (state == GameState::HUB) {
                    DialogueManager::GetInstance().Draw();
                }
            }
        EndDrawing();
    }

    // De-Initialization
    delete teamManager;
    UnloadTexture(lanceSprites.idle);
    UnloadTexture(lanceSprites.run);
    UnloadTexture(lanceSprites.weapon);
    UnloadTexture(lanceSprites.muzzleFlash);
    UnloadTexture(lanceSprites.bullet);
    UnloadTexture(lanceSprites.impact);
    UnloadTexture(lanceSprites.dashFront);
    UnloadTexture(lanceSprites.dashBack);
    UnloadTexture(keithSprites.idle);
    UnloadTexture(keithSprites.run);
    UnloadTexture(keithSprites.weapon);
    UnloadTexture(keithSprites.attack1);
    UnloadTexture(keithSprites.attack2);
    UnloadTexture(hunkSprites.idle);
    UnloadTexture(hunkSprites.run);
    UnloadTexture(hunkSprites.weapon);
    UnloadTexture(hunkSprites.muzzleFlash);
    UnloadTexture(hunkSprites.bullet);
    UnloadTexture(hunkSprites.impact);
    
    CloseWindow();

    // Note: AudioManager singleton's destructor will close the audio device automatically when main exits.

    return 0;
}
