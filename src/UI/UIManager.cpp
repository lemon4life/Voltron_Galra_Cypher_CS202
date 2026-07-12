#include "UI/UIManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/GameManager.h"
#include "Entities/Player/Paladin.h"
#include <string>
#include <vector>

UIManager::UIManager() : teamManager(nullptr) {
    statsShell.id = 0;
    statsShellBack.id = 0;
}

UIManager::~UIManager() {
    if (statsShell.id != 0) {
        UnloadTexture(statsShell);
    }
    if (statsShellBack.id != 0) {
        UnloadTexture(statsShellBack);
    }
}

void UIManager::Initialize() {
    statsShell = LoadTexture("assets/UI/Team_StatsShell.png");
    statsShellBack = LoadTexture("assets/UI/Team_StatsShell_Back.png");
}

void UIManager::DrawHUD(int screenWidth, int screenHeight) {
    if (!teamManager) return;
    DrawTeamHUD(teamManager, screenWidth, screenHeight);
}

// Helper for cropping texture to prevent stretching
void DrawTextureCropped(Texture2D texture, Rectangle destRec, Color tint) {
    if (texture.id == 0) return;
    
    float destAspect = destRec.width / destRec.height;
    float srcAspect = (float)texture.width / (float)texture.height;
    
    Rectangle sourceRec;
    if (destAspect > srcAspect) {
        // Dest is wider than source. Crop top/bottom.
        float cropHeight = texture.width / destAspect;
        sourceRec = { 0.0f, (texture.height - cropHeight) / 2.0f, (float)texture.width, cropHeight };
    } else {
        // Dest is taller than source. Crop left/right.
        float cropWidth = texture.height * destAspect;
        sourceRec = { (texture.width - cropWidth) / 2.0f, 0.0f, cropWidth, (float)texture.height };
    }
    
    DrawTexturePro(texture, sourceRec, destRec, {0,0}, 0.0f, tint);
}

void UIManager::DrawTeamHUD(TeamManager* team, int screenWidth, int screenHeight) {
    if (!team) return;
    Paladin* active = team->GetActivePaladin();
    if (!active) return;
    
    std::vector<Paladin*> roster = team->GetTeam();
    int numPaladins = roster.size();
    if (numPaladins == 0) return;

    // --- Pause Button (Top Left) ---
    Rectangle pauseBtn = { 10.0f, 10.0f, 30.0f, 30.0f };
    Vector2 rawMouse = GetMousePosition();
    Vector2 scaledMouse = { rawMouse.x * ((float)screenWidth / 1366.0f), rawMouse.y * ((float)screenHeight / 1024.0f) };
    
    bool isHovered = CheckCollisionPointRec(scaledMouse, pauseBtn);
    DrawRectangleRec(pauseBtn, isHovered ? Fade(GRAY, 0.8f) : Fade(BLACK, 0.5f));
    DrawRectangleLinesEx(pauseBtn, 2.0f, WHITE);
    DrawRectangle(pauseBtn.x + 8, pauseBtn.y + 6, 4, 18, WHITE);
    DrawRectangle(pauseBtn.x + 18, pauseBtn.y + 6, 4, 18, WHITE);
    
    if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        GameState currentState = GameManager::GetInstance().GetState();
        if (currentState == GameState::PLAYING) {
            GameManager::GetInstance().SetState(GameState::PAUSED);
        } else if (currentState == GameState::PAUSED) {
            GameManager::GetInstance().SetState(GameState::PLAYING);
        }
    }

    // Position of the Stats HUD container
    float startX = 50.0f;
    float startY = 10.0f;

    // Prepare off-field characters
    Paladin* offField1 = nullptr;
    Paladin* offField2 = nullptr;
    int activeIdx = team->GetActiveIndex();
    
    int offIndex = 0;
    for (int i = 0; i < numPaladins; i++) {
        if (i == activeIdx) continue;
        if (offIndex == 0) offField1 = roster[i];
        else if (offIndex == 1) offField2 = roster[i];
        offIndex++;
    }

    // Determine shell width based on number of paladins (doubled)
    float shellWidth = 226.0f;
    if (numPaladins == 2) shellWidth = 354.0f;
    else if (numPaladins == 3) shellWidth = 482.0f;

    // --- Layer 0: Lowest Layer (Stats Shell Back) ---
    if (statsShellBack.id != 0) {
        Rectangle sourceRec = {0, 0, shellWidth, 50.0f};
        DrawTextureRec(statsShellBack, sourceRec, {startX, startY}, WHITE);
    }

    // --- Layer 1: Portraits ---
    Texture2D actPortrait = active->GetIdleTexture();
    DrawTextureCropped(actPortrait, {startX + 4, startY + 4, 90, 42}, WHITE);

    if (offField1) {
        DrawTextureCropped(offField1->GetIdleTexture(), {startX + 226, startY + 4, 60, 28}, WHITE);
    }
    if (offField2) {
        DrawTextureCropped(offField2->GetIdleTexture(), {startX + 354, startY + 4, 60, 28}, WHITE);
    }

    // Helper lambda for HP
    auto DrawHP = [&](Paladin* p, float x, float y, float maxW, float h) {
        if (!p) return;
        float hp = p->GetHealth();
        float maxHp = p->GetMaxHealth();
        float pct = maxHp > 0 ? (hp / maxHp) : 0;
        DrawRectangle(startX + x, startY + y, (int)(maxW * pct), h, GREEN);
    };

    // --- Layer 2: HP Bars (Doubled) ---
    DrawHP(active, 98, 4, 124, 28);
    DrawHP(offField1, 290, 4, 60, 12);
    DrawHP(offField2, 418, 4, 60, 12);

    // --- Layer 3: Mask Rectangle ---
    Color maskColor = { 91, 91, 103, 255 }; // #5b5b67
    DrawRectangle(startX + 147, startY + 20, 76, 12, maskColor);

    // Helper lambda for EX Energy
    auto DrawEX = [&](Paladin* p, float x, float y, float maxW, float h) {
        if (!p) return;
        float ex = p->GetExEnergy();
        float maxEx = p->GetMaxExEnergy();
        float pct = maxEx > 0 ? (ex / maxEx) : 0;
        DrawRectangle(startX + x, startY + y, (int)(maxW * pct), h, PURPLE);
    };

    // --- Layer 4: EX Bars (Doubled) ---
    DrawEX(active, 146, 20, 76, 12);
    DrawEX(offField1, 290, 20, 60, 12);
    DrawEX(offField2, 418, 20, 60, 12);

    // --- Layer 5: Stats Shell Overlay ---
    if (statsShell.id != 0) {
        Rectangle sourceRec = {0, 0, shellWidth, 50.0f};
        DrawTextureRec(statsShell, sourceRec, {startX, startY}, WHITE);
    }
}
