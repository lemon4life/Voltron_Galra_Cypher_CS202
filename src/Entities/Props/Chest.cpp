#include "Entities/Props/Chest.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/ObjectManager.h"
#include "Entities/Player/Paladin.h"
#include "raymath.h"
#include <algorithm>

Chest::Chest(Vector2 pos)
    : GameObject(pos, GameObjectType::Prop) {
    chestBottom = AssetManager::GetInstance().GetTexture("chest_bottom");
    chestTop = AssetManager::GetInstance().GetTexture("chest_top");
    // Collision & depth sort box anchored so chest base draws behind the emerging pot
    boundingBox = { pos.x - 10.0f, pos.y - 16.0f, 20.0f, 16.0f };
}

void Chest::Update(float deltaTime) {
    // 1. Proximity Trigger: Check if player is near (< 40px)
    if (!isOpened && !isOpening) {
        Paladin* paladin = GameManager::GetInstance().GetTeamManager() 
            ? GameManager::GetInstance().GetTeamManager()->GetActivePaladin() 
            : nullptr;
        if (paladin) {
            float dist = Vector2Distance(position, paladin->GetPosition());
            if (dist < 40.0f) {
                isOpening = true;
                selectedPotType = GetRandomValue(0, 2);
                AudioManager::GetInstance().PlaySoundEffect("fx_chest_open");
            }
        }
    }

    // 2. Sliding Split-Lid Animation (slides outward to 16px total gap)
    if (isOpening && openProgress < 1.0f) {
        openProgress = std::min(1.0f, openProgress + deltaTime * 2.0f);
    }

    // 3. Emerging Pot Animation & Spawning
    if (isOpening && openProgress >= 1.0f && !potSpawned) {
        potScaleProgress = std::min(1.0f, potScaleProgress + deltaTime * 2.5f);

        if (potScaleProgress >= 1.0f) {
            MapObjectId potId = MapObjectId::PotHP;
            if (selectedPotType == 1) potId = MapObjectId::PotEX;
            else if (selectedPotType == 2) potId = MapObjectId::PotQuint;

            // Spawn the consumable pot centered on top of the chest opening
            GameManager::GetInstance().GetObjectManager().QueueSpawn(
                potId,
                { position.x, position.y - 6.0f }
            );

            potSpawned = true;
            isOpening = false;
            isOpened = true;
        }
    }
}

void Chest::Draw() {
    if (chestBottom.id == 0 || chestTop.id == 0) {
        // Fallback placeholder
        DrawRectangle((int)(position.x - 10.0f), (int)(position.y - 16.0f), 20, 32, BROWN);
        return;
    }

    // --- Layer 1 (Bottom): Chest Base (20x32) ---
    Rectangle bottomSrc = { 0.0f, 0.0f, 20.0f, 32.0f };
    Rectangle bottomDest = { position.x, position.y, 20.0f, 32.0f };
    DrawTexturePro(chestBottom, bottomSrc, bottomDest, { 10.0f, 16.0f }, 0.0f, WHITE);

    // --- Layer 2 (Middle): Sliding Split Lids (chest_top: 20x16 sliced into two 10x16 halves) ---
    // Left Lid: Slides left by 8.0f * openProgress
    Rectangle leftSrc = { 0.0f, 0.0f, 10.0f, 16.0f };
    Rectangle leftDest = { position.x - 8.0f * openProgress, position.y - 8.0f, 10.0f, 16.0f };
    DrawTexturePro(chestTop, leftSrc, leftDest, { 10.0f, 8.0f }, 0.0f, WHITE);

    // Right Lid: Slides right by 8.0f * openProgress
    Rectangle rightSrc = { 10.0f, 0.0f, 10.0f, 16.0f };
    Rectangle rightDest = { position.x + 8.0f * openProgress, position.y - 8.0f, 10.0f, 16.0f };
    DrawTexturePro(chestTop, rightSrc, rightDest, { 0.0f, 8.0f }, 0.0f, WHITE);

    // --- Layer 3 (Topmost): Emerging Pot Preview (Rises and scales up on top of BOTH chest_bottom and chest_top) ---
    if (isOpening && !potSpawned) {
        const char* potTexKey = "pot_hp";
        if (selectedPotType == 1) potTexKey = "pot_ex";
        else if (selectedPotType == 2) potTexKey = "pot_quint";

        Texture2D potTex = AssetManager::GetInstance().GetTexture(potTexKey);
        if (potTex.id != 0) {
            float totalProgress = std::clamp((openProgress * 0.3f) + (potScaleProgress * 0.7f), 0.1f, 1.0f);
            float pw = (float)potTex.width * totalProgress;
            float ph = (float)potTex.height * totalProgress;
            float potCenterY = position.y - (potScaleProgress * 6.0f);

            Rectangle src = { 0.0f, 0.0f, (float)potTex.width, (float)potTex.height };
            Rectangle dest = { position.x, potCenterY, pw, ph };
            DrawTexturePro(potTex, src, dest, { pw * 0.5f, ph * 0.5f }, 0.0f, WHITE);
        }
    }
}
