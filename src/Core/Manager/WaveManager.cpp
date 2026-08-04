#include "Core/Manager/WaveManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/EntityFactory.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
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
    if (!levelManager->IsLegacyMap()) {
        UpdateDungeonRoom(deltaTime, teamManager, levelManager);
        return;
    }
    
    // --- Legacy map combat (unchanged) ---
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
            SpawnEnemy(teamManager, levelManager);
        } else if (activeEnemies == 0) {
            if (currentWave == 5) {
                // Wave cleared, no victory state
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
    
    // Room just locked — initialize waves if we haven't yet
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
            SpawnEnemy(teamManager, levelManager);
        } else if (activeEnemies == 0) {
            // Wave cleared
            if (dungeonCurrentWave >= dungeonTotalWaves) {
                // All waves done — room cleared!
                // Room cleared!
                levelManager->SetActiveRoomState(RoomState::CLEARED);
                
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

void WaveManager::SpawnEnemy(TeamManager* teamManager, LevelManager* levelManager) {
    spawnTimer -= GetFrameTime();
    if (spawnTimer > 0.0f) return;
    
    bool spawned = false;
    int attempts = 0;
    while (!spawned && attempts < 50) {
        Rectangle bounds = levelManager->GetCurrentRoomBounds();
        float randX = bounds.x + 32.0f + (float)(rand() % (int)std::max(1.0f, bounds.width - 64.0f));
        float randY = bounds.y + 32.0f + (float)(rand() % (int)std::max(1.0f, bounds.height - 64.0f));
        Vector2 spawnPos = { randX, randY };

        if (Vector2Distance(spawnPos, teamManager->GetActivePaladin()->GetPosition()) > 100.0f) {
            char spawnType = 'E';
            
            if (isBossRoom) {
                spawnType = 'B';
            } else if (rangeEnemiesToSpawn > 0) {
                spawnType = 'R';
            } else if (diverEnemiesToSpawn > 0) {
                spawnType = 'D';
            }
            
            GameObject* newEnemy = EntityFactory::CreateEntity(
                spawnType,
                spawnPos,
                teamManager,
                levelManager->GetLevelAccessBundle()
            );
            if (newEnemy) {
                if (levelManager->IsValidSpawnLocation(newEnemy)) {
                    levelManager->AddEntity(newEnemy);
                    if (spawnType == 'R' && rangeEnemiesToSpawn > 0) rangeEnemiesToSpawn--;
                    if (spawnType == 'D' && diverEnemiesToSpawn > 0) diverEnemiesToSpawn--;
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
}

void WaveManager::DrawHUD() {
    LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
    
    if (!levelManager->IsLegacyMap()) {
        // Dungeon room HUD
        if (levelManager->GetActiveRoomState() == RoomState::LOCKED && dungeonTotalWaves > 0) {
            if (isBossRoom) {
                DrawText("BOSS BATTLE!", 240, 10, 20, RED);
            } else {
                DrawText(TextFormat("WAVE %d / %d", dungeonCurrentWave, dungeonTotalWaves), 260, 10, 18, WHITE);
            }
            
            if (showWaveTextTimer > 0.0f) {
                if (isBossRoom) {
                    DrawText("BOSS WARNING!", 200, 240, 28, RED);
                } else if (dungeonCurrentWave == 1 && enemiesToSpawn > 0) {
                    DrawText("ENEMIES INCOMING!", 200, 240, 20, RED);
                } else if (dungeonCurrentWave > 1) {
                    DrawText(TextFormat("WAVE %d!", dungeonCurrentWave), 260, 240, 20, YELLOW);
                }
            }
        } else if (showWaveTextTimer > 0.0f && dungeonTotalWaves == 0) {
            DrawText("ROOM CLEARED!", 240, 240, 20, GREEN);
        }
        return;
    }
    
    // Legacy map HUD
    if (currentWave == 0) return;
    DrawText(TextFormat("WAVE: %d / 5", currentWave), 360, 10, 20, WHITE);
    if (showWaveTextTimer > 0.0f) {
        if (currentWave == 5) {
            DrawText("BOSS WARNING!", 150, 240, 30, RED);
        } else if (currentWave == 1) {
            DrawText("WAVE 1 START!", 180, 240, 20, GREEN);
        } else {
            DrawText("WAVE CLEARED!", 180, 240, 20, GREEN);
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
