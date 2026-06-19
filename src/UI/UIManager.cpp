#include "UI/UIManager.h"
#include <string>

UIManager::UIManager() : currentHp(0), maxHp(1), currentArmor(0), maxArmor(1), isLance(true) {}

void UIManager::OnPlayerStatsChanged(int hp, int maxHp, int armor, int maxArmor, bool isLance) {
    this->currentHp = hp;
    this->maxHp = maxHp > 0 ? maxHp : 1; // Prevent division by zero
    this->currentArmor = armor;
    this->maxArmor = maxArmor > 0 ? maxArmor : 1;
    this->isLance = isLance;
}

void UIManager::DrawHUD() {
    // Top-left offset
    int startX = 10;
    int startY = 10;

    // Draw background panel
    DrawRectangle(startX, startY, 148, 56, Fade(BLACK, 0.5f));

    // Armor Bar
    float armorPercent = (float)currentArmor / maxArmor;
    DrawRectangle(startX + 10, startY + 10, 128, 16, DARKGRAY);
    DrawRectangle(startX + 10, startY + 10, (int)(128 * armorPercent), 16, SKYBLUE);

    // HP Bar
    float hpPercent = (float)currentHp / maxHp;
    DrawRectangle(startX + 10, startY + 30, 128, 16, DARKGRAY);
    DrawRectangle(startX + 10, startY + 30, (int)(128 * hpPercent), 16, GREEN);

    // Character Name
    std::string nameText = isLance ? "LANCE" : "KEITH";
    Color nameColor = isLance ? SKYBLUE : RED;
    DrawText(nameText.c_str(), startX + 148 + 10, startY + 10, 20, nameColor);
}
