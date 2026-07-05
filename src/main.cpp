#include "raylib.h"
#include "Entities/Player.h"
#include "Core/GameManager.h"
#include "Core/AudioManager.h"
#include "Core/LevelManager.h"
#include "Combat/MeleeAttackStrategy.h"
#include "Combat/RangedAttackStrategy.h"
#include "UI/UIManager.h"
#include "Core/WaveManager.h"
#include "Core/DialogueManager.h"
#include "Entities/NPC.h"
#include "raymath.h"

// Init Window config
// Window and Game Resolutions (4:3 aspect ratio landscape approx)
const int WINDOW_WIDTH = 1366;
const int WINDOW_HEIGHT = 1024;

const int GAME_WIDTH = 683;
const int GAME_HEIGHT = 512;
const int BASE_FPS = 60;

void ResetGame(Player* player, LevelManager* levelManager, WaveManager* waveManager) {
    player->SetPosition({ 256.0f, 256.0f });
    player->ResetStats();
    GameManager::GetInstance().ClearProjectiles();
    levelManager->LoadLevel("assets/levels/level1.txt", player);
    waveManager->Reset();
    GameManager::GetInstance().SetState(GameState::PLAYING);
}

int main() {
    // Initialize Window
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Voltron: Mission Galra Cypher");

    // Initialize AudioManager Singleton (Initializes Audio Device)
    AudioManager::GetInstance();

    // Load Audio
    AudioManager::GetInstance().LoadSound("shoot", "assets/audio/shoot.wav");
    AudioManager::GetInstance().LoadSound("swing", "assets/audio/swing.wav");
    AudioManager::GetInstance().LoadSound("hit", "assets/audio/hit.wav");
    AudioManager::GetInstance().LoadSound("blip", "assets/audio/blip.wav");
    AudioManager::GetInstance().LoadSound("footstep", "assets/audio/footstep.wav");
    AudioManager::GetInstance().LoadMusic("bgm", "assets/audio/bgm.mp3");

    // Initialize Dialogue Assets
    DialogueManager::GetInstance().InitializeAssets();

    // Start background music
    AudioManager::GetInstance().PlayMusicTrack("bgm");

    CharacterSprites lanceSprites;
    lanceSprites.restIdle = LoadTexture("assets/sprites/Lance/Rest_Idle.png");
    lanceSprites.restRun = LoadTexture("assets/sprites/Lance/Rest_Run.png");
    lanceSprites.battleIdle = LoadTexture("assets/sprites/Lance/Battle_Idle.png");
    lanceSprites.battleRun = LoadTexture("assets/sprites/Lance/Battle_Run.png");
    lanceSprites.weapon = LoadTexture("assets/sprites/Lance/Weapon_Static.png");

    CharacterSprites keithSprites;
    keithSprites.restIdle = LoadTexture("assets/sprites/Keith/Rest_Idle.png");
    keithSprites.restRun = LoadTexture("assets/sprites/Keith/Rest_Run.png");
    keithSprites.battleIdle = LoadTexture("assets/sprites/Keith/Battle_Idle.png");
    keithSprites.battleRun = LoadTexture("assets/sprites/Keith/Battle_Run.png");
    keithSprites.weapon = LoadTexture("assets/sprites/Keith/Weapon_Static.png");

    // Instantiate Player as a GameObject pointer
    Vector2 startPos = { (float)GAME_WIDTH / 2.0f, (float)GAME_HEIGHT / 2.0f };
    Player* player = new Player(startPos, lanceSprites, keithSprites);

    // Initialize UI Manager
    UIManager uiManager;
    player->AddObserver(&uiManager);
    
    // player constructor notifies observers, but we just added the observer. 
    // Let's manually trigger it once so the UI knows the starting state.
    player->NotifyObservers();

    // Setup LevelManager
    LevelManager levelManager;
    levelManager.LoadLevel("assets/levels/hub.txt", player);
    GameManager::GetInstance().SetLevelManager(&levelManager);

    // Equip the player with Lance's Ranged weapon to test the gun.
    player->SetWeapon(new RangedAttackStrategy(lanceSprites.weapon));

    // Initialize WaveManager
    WaveManager waveManager;

    // Initialize render texture for internal game resolution
    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT); // keep pixel art crisp

    // Initialize Camera
    Camera2D camera = { 0 };
    camera.target = { 0.0f, 0.0f };
    camera.offset = { std::round(GAME_WIDTH / 2.0f), std::round(GAME_HEIGHT / 2.0f) };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    GameManager::GetInstance().UpdateTargetFPS(BASE_FPS);

    // Main Game Loop
    while (!WindowShouldClose()) {
        // --- Update ---
        float deltaTime = GetFrameTime();
        
        // Update music stream continuously regardless of game state
        AudioManager::GetInstance().UpdateMusicStream();
        
        // Pass mouse coordinates to player for aiming
        Vector2 mouseScreen = GetMousePosition();
        Vector2 mouseInternal = { mouseScreen.x * ((float)GAME_WIDTH / (float)WINDOW_WIDTH), mouseScreen.y * ((float)GAME_HEIGHT / (float)WINDOW_HEIGHT) };
        Vector2 mouseWorld = GetScreenToWorld2D(mouseInternal, camera);
        player->SetAimTarget(mouseWorld);

        GameState state = GameManager::GetInstance().GetState();
        
        switch (state) {
            case GameState::MENU:
                if (IsKeyPressed(KEY_ENTER)) {
                    GameManager::GetInstance().SetState(GameState::HUB);
                }
                break;
            case GameState::HUB:
                if (DialogueManager::GetInstance().IsActive()) {
                    DialogueManager::GetInstance().Update(deltaTime);
                } else {
                    if (DialogueManager::GetInstance().IsMissionRequested()) {
                        DialogueManager::GetInstance().ClearMissionRequest();
                        ResetGame(player, &levelManager, &waveManager);
                        break;
                    }
                    
                    levelManager.UpdateLevel(deltaTime);
                    player->Update(deltaTime);
                    camera.target = { std::round(player->GetPosition().x), std::round(player->GetPosition().y) };
                    
                    if (IsKeyPressed(KEY_E)) {
                        for (auto* entity : GameManager::GetInstance().GetLevelEntities()) {
                            if (NPC* npc = dynamic_cast<NPC*>(entity)) {
                                if (Vector2Distance(player->GetPosition(), npc->GetPosition()) < 50.0f) {
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
                player->Update(deltaTime);
                GameManager::GetInstance().UpdateProjectiles(deltaTime);
                waveManager.Update(deltaTime, player, &levelManager);
                camera.target = { std::round(player->GetPosition().x), std::round(player->GetPosition().y) };
                
                if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                    GameManager::GetInstance().SetState(GameState::PAUSED);
                }
                if (player->GetHealth() <= 0) {
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
                    ResetGame(player, &levelManager, &waveManager);
                }
                break;
        }

        // --- Draw ---
        
        // 1. Draw to the internal render texture
        BeginTextureMode(target);
            if (state == GameState::MENU) {
                ClearBackground(DARKGRAY);
                DrawText("Voltron: Mission Galra Cypher", 70, 200, 24, WHITE);
                DrawText("Press ENTER to Start", 140, 300, 20, LIGHTGRAY);
            } else if (state == GameState::GAMEOVER) {
                ClearBackground(BLACK);
                DrawText("GAME OVER", 180, 220, 30, RED);
                DrawText("Press R to Restart", 160, 280, 20, LIGHTGRAY);
            } else {
                BeginMode2D(camera);
                
                if (state == GameState::HUB) {
                    ClearBackground(DARKGREEN);
                    levelManager.DrawLevel();
                    player->Draw();
                } else if (state == GameState::PLAYING) {
                    ClearBackground(DARKGRAY);
                    levelManager.DrawLevel();
                    player->Draw();
                    GameManager::GetInstance().DrawProjectiles();
                } else if (state == GameState::PAUSED) {
                    ClearBackground(DARKGRAY);
                    levelManager.DrawLevel();
                    player->Draw();
                    GameManager::GetInstance().DrawProjectiles();
                }
                
                EndMode2D();
                
                // Draw HUD outside of camera
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
            }
        EndTextureMode();

        // 2. Draw the internal texture to the main physical window (1024x1024)
        BeginDrawing();
            ClearBackground(BLACK); // Clear main window
            
            // Source rectangle from the render texture.
            // Note: OpenGL framebuffers are inverted vertically, so we invert the source height.
            Rectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height };
            
            // Destination rectangle on the main window.
            // The size is 1024x1024, which is exactly 2.0f times the 512x512 internal resolution.
            Rectangle destRec = { 0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT };
            
            // Origin of the destination rectangle (top-left)
            Vector2 origin = { 0.0f, 0.0f };
            
            DrawTexturePro(target.texture, sourceRec, destRec, origin, 0.0f, WHITE);
            
            // Draw high-res UI elements on top of the scaled game
            if (state == GameState::HUB) {
                DialogueManager::GetInstance().Draw();
            }
            
        EndDrawing();
    }

    // De-Initialization
    delete player;
    UnloadTexture(lanceSprites.restIdle);
    UnloadTexture(lanceSprites.restRun);
    UnloadTexture(lanceSprites.battleIdle);
    UnloadTexture(lanceSprites.battleRun);
    UnloadTexture(lanceSprites.weapon);
    UnloadTexture(keithSprites.restIdle);
    UnloadTexture(keithSprites.restRun);
    UnloadTexture(keithSprites.battleIdle);
    UnloadTexture(keithSprites.battleRun);
    UnloadTexture(keithSprites.weapon);
    UnloadRenderTexture(target);
    CloseWindow();

    // Note: AudioManager singleton's destructor will close the audio device automatically when main exits.

    return 0;
}
