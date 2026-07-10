#include "UI/UIManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/GameManager.h"
#include "Entities/Player/Paladin.h"
#include <string>
#include <vector>

UIManager::UIManager() : teamManager(nullptr) {}

void UIManager::DrawHUD(int screenWidth, int screenHeight) {
    if (!teamManager) return;
    
    Paladin* active = teamManager->GetActivePaladin();
    if (!active) return;

    // 1. Active Character (Top Left)
    int startX = 20;
    int startY = 20;
    
    // Portrait frame (Circular)
    DrawCircleLines(startX + 30, startY + 30, 30, WHITE);
    // Draw texture (using restIdle as a makeshift portrait)
    Texture2D portrait = active->GetIdleTexture();
    Rectangle sourceRec = { 0, 0, 32, 32 };
    Rectangle destRec = { (float)startX + 14, (float)startY + 14, 32, 32 };
    DrawTexturePro(portrait, sourceRec, destRec, {0,0}, 0.0f, WHITE);
    
    // Shared Armor Bar (above HP)
    int maxArmor = teamManager->GetMaxSharedArmor();
    int currentArmor = teamManager->GetSharedArmor();
    float armorPercent = maxArmor > 0 ? (float)currentArmor / maxArmor : 0.0f;
    DrawRectangle(startX + 70, startY + 12, 150, 8, DARKGRAY);
    DrawRectangle(startX + 70, startY + 12, (int)(150 * armorPercent), 8, SKYBLUE);

    // HP Bar
    int maxHp = active->GetMaxHealth();
    int hp = active->GetHealth();
    float hpPercent = maxHp > 0 ? (float)hp / maxHp : 0.0f;
    DrawRectangle(startX + 70, startY + 24, 150, 16, DARKGRAY);
    DrawRectangle(startX + 70, startY + 24, (int)(150 * hpPercent), 16, RED);
    
    // EX Energy Bar
    float maxEx = active->GetMaxExEnergy();
    float ex = active->GetExEnergy();
    float exPercent = maxEx > 0.0f ? ex / maxEx : 0.0f;
    DrawRectangle(startX + 70, startY + 44, 150, 8, DARKGRAY);
    DrawRectangle(startX + 70, startY + 44, (int)(150 * exPercent), 8, YELLOW);


    // 2. Off-Field Squad
    std::vector<Paladin*> team = teamManager->GetTeam();
    int activeIndex = teamManager->GetActiveIndex();
    
    int offFieldY = startY + 80;
    for (int i = 0; i < team.size(); ++i) {
        if (i == activeIndex) continue;
        
        Paladin* offField = team[i];
        
        // Small portrait
        DrawCircleLines(startX + 15, offFieldY + 15, 15, WHITE);
        Texture2D offPortrait = offField->GetIdleTexture();
        Rectangle offDestRec = { (float)startX + 7, (float)offFieldY + 7, 16, 16 };
        DrawTexturePro(offPortrait, sourceRec, offDestRec, {0,0}, 0.0f, WHITE);
        
        // Small HP Bar
        int offMaxHp = offField->GetMaxHealth();
        int offHp = offField->GetHealth();
        float offHpPercent = offMaxHp > 0 ? (float)offHp / offMaxHp : 0.0f;
        
        DrawRectangle(startX + 35, offFieldY + 10, 80, 10, DARKGRAY);
        DrawRectangle(startX + 35, offFieldY + 10, (int)(80 * offHpPercent), 10, RED);
        
        offFieldY += 40;
    }

    // 3. Pause Button (Top Right)
    Rectangle pauseBtn = { (float)screenWidth - 50.0f, 20.0f, 30.0f, 30.0f };
    
    // The internal resolution is GAME_WIDTH x GAME_HEIGHT, but the physical window is scaled up.
    // However, if we're rendering to the render texture, we should check against scaled mouse coords.
    Vector2 rawMouse = GetMousePosition();
    // Assuming WINDOW_WIDTH = 1366, GAME_WIDTH = 683, so scale factor is ~0.5
    Vector2 scaledMouse = { rawMouse.x * ((float)screenWidth / 1366.0f), rawMouse.y * ((float)screenHeight / 1024.0f) };
    
    // Draw button background
    bool isHovered = CheckCollisionPointRec(scaledMouse, pauseBtn);
    DrawRectangleRec(pauseBtn, isHovered ? Fade(GRAY, 0.8f) : Fade(BLACK, 0.5f));
    DrawRectangleLinesEx(pauseBtn, 2.0f, WHITE);
    
    // Draw two vertical bars
    DrawRectangle(pauseBtn.x + 8, pauseBtn.y + 6, 4, 18, WHITE);
    DrawRectangle(pauseBtn.x + 18, pauseBtn.y + 6, 4, 18, WHITE);
    
    // Handle click
    if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        GameState currentState = GameManager::GetInstance().GetState();
        if (currentState == GameState::PLAYING) {
            GameManager::GetInstance().SetState(GameState::PAUSED);
        } else if (currentState == GameState::PAUSED) {
            GameManager::GetInstance().SetState(GameState::PLAYING);
        }
    }
}
