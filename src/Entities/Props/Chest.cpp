#include "Entities/Props/Chest.h"
#include "Entities/Items/Coin.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/ObjectManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/InputManager.h"
#include "Entities/Player/Paladin.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

/// Creates a Chest instance from the supplied configuration.
Chest::Chest(Vector2 pos, ChestRewardType reward)
    : GameObject(pos, GameObjectType::Prop),
      rewardType(reward) {
    chestBottom = AssetManager::GetInstance().GetTexture("chest_bottom");
    chestTop = AssetManager::GetInstance().GetTexture("chest_top");
    // Collision & depth sort box anchored so chest base draws behind the emerging pot
    boundingBox = { pos.x - 10.0f, pos.y - 16.0f, 20.0f, 16.0f };
}

/// Advances this component's state for the current frame.
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

    // 3. Emerging Reward Animation & Spawning
    if (isOpening && openProgress >= 1.0f && !potSpawned) {
        potScaleProgress = std::min(1.0f, potScaleProgress + deltaTime * 2.5f);

        if (potScaleProgress >= 1.0f) {
            if (rewardType == ChestRewardType::Pot) {
                MapObjectId potId = MapObjectId::PotHP;
                if (selectedPotType == 1) potId = MapObjectId::PotEX;
                else if (selectedPotType == 2) potId = MapObjectId::PotQuint;

                // Spawn the consumable pot centered on top of the chest opening
                GameManager::GetInstance().GetObjectManager().QueueSpawn(
                    potId,
                    { position.x, position.y - 6.0f }
                );
            } else if (rewardType == ChestRewardType::Coins) {
                // Burst spawn 5 to 10 Coin pickups outward in a radial arc
                int coinCount = GetRandomValue(5, 10);
                for (int i = 0; i < coinCount; ++i) {
                    float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
                    float speed = (float)GetRandomValue(80, 180);
                    Vector2 coinVel = { std::cos(angle) * speed, std::sin(angle) * speed - 60.0f };
                    GameManager::GetInstance().GetObjectManager().AddObject(
                        std::make_unique<Coin>(Vector2{ position.x, position.y - 6.0f }, coinVel)
                    );
                }
            } else if (rewardType == ChestRewardType::Cypher) {
                // Cypher stays floating above the chest awaiting player retrieval
            }

            potSpawned = true;
            isOpening = false;
            isOpened = true;
        }
    }

    // 4. Cypher Interaction: While opened and not yet collected, player can press F to retrieve code
    if (rewardType == ChestRewardType::Cypher && isOpened && !cypherCollected) {
        cypherHoverTimer += deltaTime;
        Paladin* paladin = GameManager::GetInstance().GetTeamManager() 
            ? GameManager::GetInstance().GetTeamManager()->GetActivePaladin() 
            : nullptr;
        if (paladin) {
            float dist = Vector2Distance(position, paladin->GetPosition());
            if (dist < 50.0f && InputManager::IsInteractPressed()) {
                cypherCollected = true;
                AudioManager::GetInstance().PlaySoundEffect("fx_get_buff");

                // Spawn the mission exit gate in the fixed position in the boss room (South/Center of room)
                LevelManager* lm = GameManager::GetInstance().GetLevelManager();
                if (lm) {
                    Vector2 gatePos = { position.x, position.y + 100.0f };
                    auto bossNode = lm->GetCurrentlyLockedRoom();
                    if (!bossNode) {
                        for (const auto& node : lm->GetLevelMap().generatedNodes) {
                            if (node && node->type == RoomType::BOSS) {
                                bossNode = node;
                                break;
                            }
                        }
                    }
                    if (bossNode) {
                        Rectangle bounds = bossNode->GetWorldBounds();
                        gatePos = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f + 40.0f };
                    }
                    lm->SpawnBossExitGate(gatePos);

                    Texture2D smoke = AssetManager::GetInstance().GetTexture("AppearSmoke");
                    Texture2D light = AssetManager::GetInstance().GetTexture("AppearLight");
                    GameManager::GetInstance().AddEffect(gatePos, smoke, 5, 0.5f);
                    GameManager::GetInstance().AddEffect(gatePos, light, 5, 0.5f);
                    AudioManager::GetInstance().PlaySoundEffect("fx_show_up");
                }
            }
        }
    }
}

/// Renders this component using its current state and visual resources.
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

    // --- Layer 3 (Topmost): Emerging Pot Preview (Only for ChestRewardType::Pot) ---
    if (isOpening && !potSpawned && rewardType == ChestRewardType::Pot) {
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
    } else if (rewardType == ChestRewardType::Cypher && !cypherCollected && (isOpening || isOpened)) {
        Texture2D cypherTex = AssetManager::GetInstance().GetTexture("cypher");
        if (cypherTex.id != 0) {
            float totalProgress = std::clamp((openProgress * 0.3f) + (potScaleProgress * 0.7f), 0.1f, 1.0f);
            float cw = (float)cypherTex.width * totalProgress;
            float ch = (float)cypherTex.height * totalProgress;
            float hover = isOpened ? std::sin(cypherHoverTimer * 3.5f) * 3.0f : 0.0f;
            float cypherCenterY = position.y - 14.0f - (potScaleProgress * 6.0f) + hover;

            Rectangle src = { 0.0f, 0.0f, (float)cypherTex.width, (float)cypherTex.height };
            Rectangle dest = { position.x, cypherCenterY, cw, ch };
            
            // Subtle glowing halo behind floating cypher
            if (isOpened) {
                float pulse = 0.5f + 0.5f * std::sin(cypherHoverTimer * 4.0f);
                DrawCircleGradient((int)position.x, (int)cypherCenterY, 14.0f, ColorAlpha(SKYBLUE, 0.35f * pulse), ColorAlpha(DARKBLUE, 0.0f));
            }
            DrawTexturePro(cypherTex, src, dest, { cw * 0.5f, ch * 0.5f }, 0.0f, WHITE);
        }
    }
}
