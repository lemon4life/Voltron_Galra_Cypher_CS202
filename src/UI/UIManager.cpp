#include "UI/UIManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include <string>
#include <vector>

namespace {
constexpr Rectangle PAUSE_BUTTON_BOUNDS = {10.0f, 10.0f, 30.0f, 30.0f};
}

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

bool UIManager::IsPauseButtonPressed(Vector2 mousePosition) const {
    return CheckCollisionPointRec(mousePosition, PAUSE_BUTTON_BOUNDS) &&
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void UIManager::DrawHUD(
    int screenWidth,
    int screenHeight,
    Vector2 mousePosition
) {
    if (!teamManager) return;
    DrawTeamHUD(teamManager, screenWidth, screenHeight, mousePosition);
}

void DrawPortrait(Paladin* p, Rectangle destRec) {
    if (!p) return;
    Texture2D texture = p->GetIdleTexture();
    if (texture.id == 0) return;
    
    // Idle sprite has 4 frames
    float frameWidth = (float)texture.width / 4.0f;
    float frameHeight = (float)texture.height;
    
    // We only want the top half of the first frame
    Rectangle baseSourceRec = { 0.0f, 2.0f, frameWidth, frameHeight / 2.0f + 2 };
    
    float destAspect = destRec.width / destRec.height;
    float srcAspect = baseSourceRec.width / baseSourceRec.height;
    
    Rectangle sourceRec;
    if (destAspect > srcAspect) {
        // Dest is wider than source. Crop top/bottom.
        float cropHeight = baseSourceRec.width / destAspect;
        sourceRec = { 
            baseSourceRec.x, 
            baseSourceRec.y + (baseSourceRec.height - cropHeight) / 2.0f, 
            baseSourceRec.width, 
            cropHeight 
        };
    } else {
        // Dest is taller than source. Crop left/right.
        float cropWidth = baseSourceRec.height * destAspect;
        sourceRec = { 
            baseSourceRec.x + (baseSourceRec.width - cropWidth) / 2.0f, 
            baseSourceRec.y, 
            cropWidth, 
            baseSourceRec.height 
        };
    }
    
    Color tint = p->GetHealth() <= 0 ? DARKGRAY : WHITE;
    DrawTexturePro(texture, sourceRec, destRec, {0,0}, 0.0f, tint);
}

void UIManager::DrawTeamHUD(
    TeamManager* team,
    int screenWidth,
    int screenHeight,
    Vector2 mousePosition
) {
    if (!team) return;
    Paladin* active = team->GetActivePaladin();
    if (!active) return;
    
    std::vector<Paladin*> roster = team->GetTeam();
    int numPaladins = roster.size();
    if (numPaladins == 0) return;

    // --- Pause Button (Top Left) ---
    const Rectangle pauseBtn = PAUSE_BUTTON_BOUNDS;
    bool isHovered = CheckCollisionPointRec(mousePosition, pauseBtn);
    DrawRectangleRec(pauseBtn, isHovered ? Fade(GRAY, 0.8f) : Fade(BLACK, 0.5f));
    DrawRectangleLinesEx(pauseBtn, 2.0f, WHITE);
    DrawRectangle(pauseBtn.x + 8, pauseBtn.y + 6, 4, 18, WHITE);
    DrawRectangle(pauseBtn.x + 18, pauseBtn.y + 6, 4, 18, WHITE);
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
    DrawPortrait(active, {startX + 4, startY + 4, 90, 42});

    if (offField1) {
        DrawPortrait(offField1, {startX + 226, startY + 4, 60, 28});
    }
    if (offField2) {
        DrawPortrait(offField2, {startX + 354, startY + 4, 60, 28});
    }

    // Helper lambda for HP
    auto DrawHP = [&](Paladin* p, float x, float y, float maxW, float h) {
        if (!p || p->GetHealth() <= 0) return; // Don't draw bars if downed
        float hp = p->GetHealth();
        float ghost = p->GetGhostHp();
        float maxHp = p->GetMaxHealth();
        
        float pctGhost = maxHp > 0 ? (ghost / maxHp) : 0.0f;
        float pctReal = maxHp > 0 ? (hp / maxHp) : 0.0f;
        
        // Draw Ghost HP (Red Base)
        DrawRectangle(startX + x, startY + y, (int)(maxW * pctGhost), h, RED);
        
        // Draw Real HP (Green Overlay)
        DrawRectangle(startX + x, startY + y, (int)(maxW * pctReal), h, GREEN);
    };

    // --- Layer 2: HP Bars (Doubled) ---
    DrawHP(active, 98, 4, 124, 28);
    DrawHP(offField1, 290, 4, 60, 12);
    DrawHP(offField2, 418, 4, 60, 12);

    // --- Layer 3: Mask Rectangle ---
    Color maskColor = { 91, 91, 103, 255 }; // #5b5b67
    DrawRectangle(startX + 146, startY + 20, 76, 12, maskColor);

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

    // --- Active HP Text ---
    char hpText[32];
    snprintf(hpText, sizeof(hpText), "%d/%d", active->GetHealth(), active->GetMaxHealth());
    
    int fontSize = 10;
    int textWidth = MeasureText(hpText, fontSize);
    int textX = startX + 98 + (44 - textWidth) / 2;
    int textY = startY + 34 + (12 - fontSize) / 2;
    DrawText(hpText, textX, textY, fontSize, WHITE);

}

// --- Core UI Helpers ---

void UIManager::DrawModalOverlay() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));
}

void UIManager::DrawPopupFrame(Rectangle bounds, const char* title) {
    // Background and border
    DrawRectangleRounded(bounds, 0.1f, 16, Fade(DARKGRAY, 0.95f));
    DrawRectangleRoundedLinesEx(bounds, 0.1f, 16, 2.0f, BLACK);

    // Title area
    int titleWidth = MeasureText(title, 24);
    int titleX = bounds.x + (bounds.width - titleWidth) / 2;
    int titleY = bounds.y + 20;

    DrawText(title, titleX, titleY, 24, WHITE);
    DrawLine(bounds.x + 20, bounds.y + 60, bounds.x + bounds.width - 20, bounds.y + 60, GRAY);
}
