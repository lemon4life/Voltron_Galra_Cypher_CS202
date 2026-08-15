#include "UI/UIManager.h"
#include "UI/UIUtils.h"
#include "Core/Constants.h"
#include "Core/Manager/InputManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "UI/PaladinPortrait.h"
#include "Core/Manager/AssetManager.h"
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
    int offField1Idx = -1;
    int offField2Idx = -1;
    for (int i = 0; i < numPaladins; i++) {
        if (i == activeIdx) continue;
        if (offIndex == 0) { offField1 = roster[i]; offField1Idx = i; }
        else if (offIndex == 1) { offField2 = roster[i]; offField2Idx = i; }
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
    
    Font fontMono = AssetManager::GetInstance().GetCustomFont("PixeloidMono");
    Font fontSans = AssetManager::GetInstance().GetCustomFont("PixeloidSans");
    Font fontBold = AssetManager::GetInstance().GetCustomFont("PixeloidBold");

    auto DrawPortraitNumber = [&](int idx, Rectangle dest) {
        int num = idx + 1;
        DrawCircle(dest.x + dest.width - 8, dest.y + dest.height - 8, 8, Fade(BLACK, 0.7f));
        Vector2 numSize = MeasureTextEx(fontMono, TextFormat("%d", num), 10, 1.0f);
        UIUtils::DrawText("PixeloidMono", TextFormat("%d", num), { dest.x + dest.width - 8 - numSize.x/2, dest.y + dest.height - 8 - numSize.y/2 }, static_cast<UIUtils::FontSize>(10), WHITE);
    };

    Rectangle activeDest = {startX + 4, startY + 4, 90, 42};
    DrawPaladinPortrait(active, activeDest);
    DrawPortraitNumber(activeIdx, activeDest);

    if (offField1) {
        Rectangle off1Dest = {startX + 226, startY + 4, 60, 28};
        DrawPaladinPortrait(offField1, off1Dest);
        DrawPortraitNumber(offField1Idx, off1Dest);
    }
    if (offField2) {
        Rectangle off2Dest = {startX + 354, startY + 4, 60, 28};
        DrawPaladinPortrait(offField2, off2Dest);
        DrawPortraitNumber(offField2Idx, off2Dest);
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

    // Helper lambda for EX Energy (BLUE — for Skills)
    auto DrawEX = [&](Paladin* p, float x, float y, float maxW, float h) {
        if (!p) return;
        float ex = p->GetExEnergy();
        float maxEx = p->GetMaxExEnergy();
        float pct = maxEx > 0 ? (ex / maxEx) : 0;
        DrawRectangle(startX + x, startY + y, (int)(maxW * pct), h, BLUE);
    };

    // --- Layer 4: EX Bars (BLUE) ---
    DrawEX(active, 146, 20, 76, 12);
    DrawEX(offField1, 290, 20, 60, 12);
    DrawEX(offField2, 418, 20, 60, 12);

    // --- Layer 4.5: Quintessence Bar (PURPLE — shared team ultimate fuel, 3 cells) ---
    Rectangle qBar = { startX + 146, startY + 36, 332, 12 };
    UIUtils::DrawSegmentedProgressBar(
        qBar,
        team->GetQuintessence(),
        team->GetMaxQuintessence(),
        3, DARKGRAY, PURPLE, RAYWHITE
    );

    // --- Layer 5: Stats Shell Overlay ---
    if (statsShell.id != 0) {
        Rectangle sourceRec = {0, 0, shellWidth, 50.0f};
        DrawTextureRec(statsShell, sourceRec, {startX, startY}, WHITE);
    }

    // --- Active HP Text ---
    char hpText[32];
    snprintf(hpText, sizeof(hpText), "%d/%d", active->GetHealth(), active->GetMaxHealth());
    
    int fontSize = 10;
    Vector2 textSize = MeasureTextEx(fontMono, hpText, fontSize, 1.0f);
    float textX = startX + 98 + (44 - textSize.x) / 2;
    float textY = startY + 34 + (12 - textSize.y) / 2;
    UIUtils::DrawText("PixeloidMono", hpText, { textX, textY }, static_cast<UIUtils::FontSize>(fontSize), WHITE);

    if (InputManager::GetMode() == InputMode::KEYBOARD_ONLY && !Constants::isAutoAimEnabled) {
        const char* hint = "Auto-Aim ('T') Recommended for Keyboard Only";
        Vector2 hintSize = MeasureTextEx(fontSans, hint, 20, 1.0f);
        UIUtils::DrawText("PixeloidSans", hint, { (GetScreenWidth() - hintSize.x) / 2, GetScreenHeight() - 100.0f }, static_cast<UIUtils::FontSize>(20), Fade(WHITE, 0.7f));
    }


}

// --- Core UI Helpers ---

void UIManager::DrawModalOverlay() {
    DrawRectangle(0, 0, Constants::GAME_WIDTH, Constants::GAME_HEIGHT, Fade(BLACK, 0.6f));
}

void UIManager::DrawPopupFrame(Rectangle bounds, const char* title) {
    // Background and border
    DrawRectangleRounded(bounds, 0.1f, 16, Fade(DARKGRAY, 0.95f));
    DrawRectangleRoundedLinesEx(bounds, 0.1f, 16, 2.0f, BLACK);

    // Title area
    Font fontBold = AssetManager::GetInstance().GetCustomFont("PixeloidBold");
    Vector2 titleSize = MeasureTextEx(fontBold, title, 24, 1.0f);
    float titleX = bounds.x + (bounds.width - titleSize.x) / 2;
    float titleY = bounds.y + 20;

    UIUtils::DrawText("PixeloidBold", title, { titleX, titleY }, static_cast<UIUtils::FontSize>(24), WHITE);
    DrawLine(bounds.x + 20, bounds.y + 60, bounds.x + bounds.width - 20, bounds.y + 60, GRAY);
}
