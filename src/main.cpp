#include "raylib.h"
#include "Entities/Player.h"

int main() {
    // Window and Game Resolutions
    const int windowWidth = 1024;
    const int windowHeight = 1024;

    const int gameWidth = 512;
    const int gameHeight = 512;

    // Initialize Window
    InitWindow(windowWidth, windowHeight, "Voltron: Mission Galra Cypher");

    // Load Textures
    Texture2D texIdle = LoadTexture("assets/sprites/Lance_Idle.png");
    Texture2D texRun = LoadTexture("assets/sprites/Lance_Fight_Run.png");

    // Instantiate Player as a GameObject pointer
    Vector2 startPos = { (float)gameWidth / 2.0f, (float)gameHeight / 2.0f };
    GameObject* player = new Player(startPos, texIdle, texRun);

    // Create Render Texture for internal game resolution
    RenderTexture2D target = LoadRenderTexture(gameWidth, gameHeight);
    
    // Use Point filtering to keep pixel art crisp during scaling
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    SetTargetFPS(60);

    // Main Game Loop
    while (!WindowShouldClose()) {
        // --- Update ---
        float deltaTime = GetFrameTime();
        
        // Polymorphic Update
        player->Update(deltaTime);

        // --- Draw ---
        
        // 1. Draw to the internal render texture (512x512)
        BeginTextureMode(target);
            ClearBackground(DARKGRAY); // Clear internal texture to dark gray
            
            // Polymorphic Draw
            player->Draw();
            
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

    return 0;
}
