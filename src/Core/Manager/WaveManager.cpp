#include "Core/Manager/WaveManager.h"
#include "UI/UIUtils.h"
#include "Core/Manager/LevelManager.h"
#include "Core/EntityFactory.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Level/RoomNode.h"
#include "raymath.h"
#include <algorithm>
#include <cstdlib>

WaveManager::WaveManager() {
    Reset();
}

void WaveManager::Reset(
    int startingEnemies,
    int startingRangeEnemies,
    int startingDiverEnemies
) {
    currentWave = startingEnemies == 0 ? 0 : 1;
    enemiesToSpawn = startingEnemies;
    rangeEnemiesToSpawn = std::clamp(startingRangeEnemies, 0, startingEnemies);
    diverEnemiesToSpawn = std::clamp(
        startingDiverEnemies,
        0,
        startingEnemies - rangeEnemiesToSpawn
    );
    spawnTimer = 0.0f;
    timeBetweenWaves = 3.0f;
    showWaveTextTimer = 2.0f;

    // Dungeon room state
    dungeonTotalWaves = 0;
    dungeonCurrentWave = 0;
    isBossRoom = false;
}

void WaveManager::Update(float deltaTime, TeamManager* teamManager, LevelManager* levelManager) {
    // --- Procedural dungeon room combat ---
    if (levelManager->IsProceduralDungeon()) {
        UpdateDungeonRoom(deltaTime, teamManager, levelManager);
        return;
    }

    // --- Layered static-map combat ---
    int activeEnemies = 0;
    for (auto* entity : levelManager->GetEntities()) {
        if (entity->GetObjectType() == GameObjectType::Enemy) {
            activeEnemies++;
        }
    }

    if (showWaveTextTimer > 0.0f) {
        showWaveTextTimer -= deltaTime;
    }

    if (timeBetweenWaves > 0.0f) {
        timeBetweenWaves -= deltaTime;
    } else {
        if (enemiesToSpawn > 0) {
            SpawnEnemy(deltaTime, teamManager, levelManager);
        } else if (activeEnemies == 0) {
            if (currentWave == 5) {
                GameManager::GetInstance().SetState(GameState::VICTORY);
            } else {
                currentWave++;
                if (currentWave == 5) {
                    enemiesToSpawn = 1;
                    AudioManager::GetInstance().PlayMusicTrack("bgm_boss_theme", 1.5f);
                } else {
                    enemiesToSpawn = currentWave;
                }
                timeBetweenWaves = 3.0f;
                showWaveTextTimer = 3.0f;
            }
        }
    }
}

void WaveManager::UpdateDungeonRoom(float deltaTime, TeamManager* teamManager, LevelManager* levelManager) {
    if (levelManager->GetActiveRoomState() != RoomState::LOCKED) {
        // Show "ROOM CLEARED" briefly after clearing
        if (showWaveTextTimer > 0.0f) {
            showWaveTextTimer -= deltaTime;
        }
        return;
    }

    // Room just locked â€” initialize waves if we haven't yet
    if (dungeonTotalWaves == 0) {
        // Check if this is a boss room
        for (const auto& node : levelManager->GetLevelMap().generatedNodes) {
            if (node->state == RoomState::LOCKED) {
                isBossRoom = (node->type == RoomType::BOSS);
                break;
            }
        }

        if (isBossRoom) {
            // Boss room: 1 wave with 1 boss
            dungeonTotalWaves = 1;
            dungeonCurrentWave = 1;
            enemiesToSpawn = 1;
            rangeEnemiesToSpawn = 0;
            diverEnemiesToSpawn = 0;
            AudioManager::GetInstance().PlayMusicTrack("bgm_boss_theme", 1.5f);
        } else {
            // Normal battle room: 3 waves of 1-4 enemies each
            dungeonTotalWaves = 3;
            dungeonCurrentWave = 1;
            enemiesToSpawn = 1 + (rand() % 4); // 1-4
            rangeEnemiesToSpawn = enemiesToSpawn / 3;
            diverEnemiesToSpawn = enemiesToSpawn / 4;
        }

        currentWave = 1;
        timeBetweenWaves = 1.5f;
        showWaveTextTimer = 2.0f;
        return;
    }

    // Count active enemies
    int activeEnemies = 0;
    for (auto* entity : levelManager->GetEntities()) {
        if (entity->GetObjectType() == GameObjectType::Enemy) {
            activeEnemies++;
        }
    }

    if (showWaveTextTimer > 0.0f) {
        showWaveTextTimer -= deltaTime;
    }

    if (timeBetweenWaves > 0.0f) {
        timeBetweenWaves -= deltaTime;
    } else {
        if (enemiesToSpawn > 0) {
            SpawnEnemy(deltaTime, teamManager, levelManager);
        } else if (activeEnemies == 0) {
            // Wave cleared
            if (dungeonCurrentWave >= dungeonTotalWaves) {
                // All waves done â€” room cleared!
                levelManager->SetActiveRoomState(RoomState::CLEARED);

                if (isBossRoom) {
                    // Boss defeated — room cleared, player can continue
                    AudioManager::GetInstance().PlayMusicTrack("bgm_battle", 1.0f);
                }

                // Reset for next room
                dungeonTotalWaves = 0;
                dungeonCurrentWave = 0;
                currentWave = 0;
                isBossRoom = false;
                showWaveTextTimer = 1.5f;
            } else {
                // Next wave
                dungeonCurrentWave++;
                currentWave = dungeonCurrentWave;

                if (isBossRoom) {
                    enemiesToSpawn = 1;
                } else {
                    enemiesToSpawn = 1 + (rand() % 4); // 1-4 random
                    rangeEnemiesToSpawn = enemiesToSpawn / 3;
                    diverEnemiesToSpawn = enemiesToSpawn / 4;
                }

                timeBetweenWaves = 2.0f;
                showWaveTextTimer = 2.0f;
            }
        }
    }
}

void WaveManager::SpawnEnemy(
    float deltaTime,
    TeamManager* teamManager,
    LevelManager* levelManager
) {
    spawnTimer -= deltaTime;
    if (spawnTimer > 0.0f) {
        return;
    }

    bool spawned = false;
    int attempts = 0;
    while (!spawned && attempts < 10) {
        Vector2 spawnPos;
        bool foundSafePos = false;
        
        if (levelManager->IsProceduralDungeon()) {
            foundSafePos = levelManager->GetSafeSpawnPosition(levelManager->GetCurrentlyLockedRoom(), spawnPos);
        } else {
            Rectangle bounds = levelManager->GetCurrentRoomBounds();
            spawnPos.x = bounds.x + 32.0f + (float)(rand() % (int)std::max(1.0f, bounds.width - 64.0f));
            spawnPos.y = bounds.y + 32.0f + (float)(rand() % (int)std::max(1.0f, bounds.height - 64.0f));
            foundSafePos = true;
        }

        if (!foundSafePos) {
            // Failed to find safe spot, skip this enemy
            if (rangeEnemiesToSpawn > 0) rangeEnemiesToSpawn--;
            else if (diverEnemiesToSpawn > 0) diverEnemiesToSpawn--;
            enemiesToSpawn--;
            spawnTimer = 0.07f;
            return;
        }

        if (Vector2Distance(
                spawnPos,
                teamManager->GetActivePaladin()->GetPosition()
            ) > 150.0f) {
            MapObjectId spawnType = MapObjectId::Chaser;

            if (isBossRoom ||
                (!levelManager->IsProceduralDungeon() && currentWave == 5)) {
                spawnType = MapObjectId::Boss;
            } else if (rangeEnemiesToSpawn > 0) {
                spawnType = MapObjectId::Range;
            } else if (diverEnemiesToSpawn > 0) {
                spawnType = MapObjectId::Diver;
            } else if (!levelManager->IsProceduralDungeon() &&
                       currentWave >= 2 && currentWave <= 4 &&
                       enemiesToSpawn == currentWave) {
                spawnType = MapObjectId::Range;
            } else if (!levelManager->IsProceduralDungeon() &&
                       currentWave >= 3 && currentWave <= 4 &&
                       enemiesToSpawn == currentWave - 1) {
                spawnType = MapObjectId::Diver;
            }

            GameObject* newEnemy = EntityFactory::CreateEntity(
                spawnType,
                spawnPos,
                {-1, -1},
                teamManager,
                levelManager->GetLevelAccessBundle()
            );
            if (newEnemy) {
                // Apply Roguelike scaling buff
                int currentFloor = GameManager::GetInstance().GetCurrentFloor();
                float floorMultiplier = 1.0f + ((currentFloor - 1) * 0.3f);
                static_cast<Enemy*>(newEnemy)->ApplyStatMultiplier(floorMultiplier);

                if (levelManager->IsValidSpawnLocation(newEnemy)) {
                    levelManager->AddEntity(newEnemy);
                    if (spawnType == MapObjectId::Range &&
                        rangeEnemiesToSpawn > 0) {
                        rangeEnemiesToSpawn--;
                    }
                    if (spawnType == MapObjectId::Diver &&
                        diverEnemiesToSpawn > 0) {
                        diverEnemiesToSpawn--;
                    }
                    enemiesToSpawn--;
                    spawnTimer = 0.07f;
                    spawned = true;
                } else {
                    delete newEnemy;
                }
            }
        }
        attempts++;
    }
    
    if (!spawned) {
        // Skip it if we couldn't spawn it safely after max attempts
        if (rangeEnemiesToSpawn > 0) rangeEnemiesToSpawn--;
        else if (diverEnemiesToSpawn > 0) diverEnemiesToSpawn--;
        enemiesToSpawn--;
        spawnTimer = 0.07f;
    }
}

void WaveManager::DrawHUD() {
    LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();

    Font fontBold = AssetManager::GetInstance().GetCustomFont("PixeloidBold");
    Font fontSans = AssetManager::GetInstance().GetCustomFont("PixeloidSans");
    Font fontMono = AssetManager::GetInstance().GetCustomFont("PixeloidMono");

    if (levelManager->IsProceduralDungeon()) {
        // Dungeon room HUD
        if (levelManager->GetActiveRoomState() == RoomState::LOCKED && dungeonTotalWaves > 0) {
            if (isBossRoom) {
                UIUtils::DrawText("PixeloidBold", "BOSS BATTLE!", { 240.0f, 10.0f }, static_cast<UIUtils::FontSize>(20), RED);
            } else {
                UIUtils::DrawText("PixeloidMono", TextFormat("WAVE %d / %d", dungeonCurrentWave, dungeonTotalWaves), { 260.0f, 10.0f }, static_cast<UIUtils::FontSize>(18), WHITE);
            }

            if (showWaveTextTimer > 0.0f) {
                if (isBossRoom) {
                    UIUtils::DrawText("PixeloidBold", "BOSS WARNING!", { 200.0f, 240.0f }, static_cast<UIUtils::FontSize>(28), RED);
                } else if (dungeonCurrentWave == 1 && enemiesToSpawn > 0) {
                    UIUtils::DrawText("PixeloidBold", "ENEMIES INCOMING!", { 200.0f, 240.0f }, static_cast<UIUtils::FontSize>(20), RED);
                } else if (dungeonCurrentWave > 1) {
                    UIUtils::DrawText("PixeloidBold", TextFormat("WAVE %d!", dungeonCurrentWave), { 260.0f, 240.0f }, static_cast<UIUtils::FontSize>(20), YELLOW);
                }
            }
        } else if (showWaveTextTimer > 0.0f && dungeonTotalWaves == 0) {
            UIUtils::DrawText("PixeloidBold", "ROOM CLEARED!", { 240.0f, 240.0f }, static_cast<UIUtils::FontSize>(20), GREEN);
        }
        return;
    }

    // Layered static-map HUD
    if (currentWave == 0) return;
    UIUtils::DrawText("PixeloidMono", TextFormat("WAVE: %d / 5", currentWave), { 360.0f, 10.0f }, static_cast<UIUtils::FontSize>(20), WHITE);
    if (showWaveTextTimer > 0.0f) {
        if (currentWave == 5) {
            UIUtils::DrawText("PixeloidBold", "BOSS WARNING!", { 150.0f, 240.0f }, static_cast<UIUtils::FontSize>(30), RED);
        } else if (currentWave == 1) {
            UIUtils::DrawText("PixeloidBold", "WAVE 1 START!", { 180.0f, 240.0f }, static_cast<UIUtils::FontSize>(20), GREEN);
        } else {
            UIUtils::DrawText("PixeloidBold", "WAVE CLEARED!", { 180.0f, 240.0f }, static_cast<UIUtils::FontSize>(20), GREEN);
        }
    }
}

void WaveManager::StartRoomWaves(int totalEnemies) {
    Reset(totalEnemies, totalEnemies / 3, totalEnemies / 4);
    timeBetweenWaves = 1.5f;
    showWaveTextTimer = 2.0f;
}

bool WaveManager::IsRoomCleared() const {
    return enemiesToSpawn <= 0 && currentWave > 0;
}
