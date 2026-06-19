#include "raylib.h"
#include "Entities/Player.h"
#include "Core/GameManager.h"
#include "Core/AudioManager.h"
#include "Core/LevelManager.h"
#include "Combat/MeleeAttackStrategy.h"
#include "Combat/RangedAttackStrategy.h"
#include "UI/UIManager.h"

int main() {
    // Window and Game Resolutions
    const int windowWidth = 1024;
    const int windowHeight = 1024;

    const int gameWidth = 512;
    const int gameHeight = 512;

    // Initialize Window
    InitWindow(windowWidth, windowHeight, "Voltron: Mission Galra Cypher");

    // Initialize AudioManager Singleton (Initializes Audio Device)
    AudioManager::GetInstance();

    // Load Textures
    Texture2D texIdle = LoadTexture("assets/sprites/Lance_Run_No_Arm.png"); // Use armless for idle too since we draw arm over it
    Texture2D texRun = LoadTexture("assets/sprites/Lance_Run_No_Arm.png");
    Texture2D texGun = LoadTexture("assets/sprites/Firearm-Arm.png");

    // Instantiate Player as a GameObject pointer
    Vector2 startPos = { (float)gameWidth / 2.0f, (float)gameHeight / 2.0f };
    Player* player = new Player(startPos, texIdle, texRun, texGun);

    // Initialize UI Manager
    UIManager uiManager;
    player->AddObserver(&uiManager);
    
    // player constructor notifies observers, but we just added the observer. 
    // Let's manually trigger it once so the UI knows the starting state.
    player->NotifyObservers();

    // Initialize LevelManager and load the level
    LevelManager levelManager;
    levelManager.LoadLevel("assets/levels/level1.txt", player);
    GameManager::GetInstance().SetLevelManager(&levelManager);

    // Equip the player with Lance's Ranged weapon to test the gun.
    player->SetWeapon(new RangedAttackStrategy(texGun));

    // Create Render Texture for internal game resolution
    RenderTexture2D target = LoadRenderTexture(gameWidth, gameHeight);
    
    // Use Point filtering to keep pixel art crisp during scaling
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    SetTargetFPS(60);

    // Main Game Loop
    while (!WindowShouldClose()) {
        // --- Update ---
        float deltaTime = GetFrameTime();
        
        // Only update gameplay/entities when GameState is PLAYING
        if (GameManager::GetInstance().GetState() == GameState::PLAYING) {
            levelManager.UpdateLevel(deltaTime);
            player->Update(deltaTime);
            GameManager::GetInstance().UpdateProjectiles(deltaTime);
        }

        // --- Draw ---
        
        // 1. Draw to the internal render texture (512x512)
        BeginTextureMode(target);
            ClearBackground(DARKGRAY); // Clear internal texture to dark gray
            
            // Only draw gameplay/entities when GameState is PLAYING
            if (GameManager::GetInstance().GetState() == GameState::PLAYING) {
                levelManager.DrawLevel();
                player->Draw();
                GameManager::GetInstance().DrawProjectiles();
                uiManager.DrawHUD();
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
            Rectangle destRec = { 0.0f, 0.0f, (float)windowWidth, (float)windowHeight };
            
            // Origin of the destination rectangle (top-left)
            Vector2 origin = { 0.0f, 0.0f };
            
            DrawTexturePro(target.texture, sourceRec, destRec, origin, 0.0f, WHITE);
            
        EndDrawing();
    }

    // De-Initialization
    delete player;
    UnloadTexture(texIdle);
    UnloadTexture(texRun);
    UnloadRenderTexture(target);
    CloseWindow();

    // Note: AudioManager singleton's destructor will close the audio device automatically when main exits.

    return 0;
}
