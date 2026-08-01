#include "Core/Manager/WaveManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/EntityFactory.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "Core/Manager/GameManager.h"
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
    currentWave = 1;
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
}

void WaveManager::Update(float deltaTime, TeamManager* teamManager, LevelManager* levelManager) {
    // Determine active enemies dynamically
    int activeEnemies = 0;
    for (auto* entity : levelManager->GetEntities()) {
        if (entity->GetObjectType() == GameObjectType::Enemy) {
            activeEnemies++;
        }
    }

    if (showWaveTextTimer > 0.0f) {
        showWaveTextTimer -= deltaTime;
    }

    // Tick down the wave delay timer
    if (timeBetweenWaves > 0.0f) {
        timeBetweenWaves -= deltaTime;
    } else {
        if (enemiesToSpawn > 0) {
            spawnTimer -= deltaTime;
            if (spawnTimer <= 0.0f) {
                // Find valid spawn location
                bool spawned = false;
                int attempts = 0;
                while (!spawned && attempts < 50) {
                    // Random pos in 15x15 grid (approx 32 to 480)
                    float randX = 32.0f + (float)(rand() % 448);
                    float randY = 32.0f + (float)(rand() % 448);
                    Vector2 spawnPos = { randX, randY };

                    if (Vector2Distance(spawnPos, teamManager->GetActivePaladin()->GetPosition()) > 150.0f) {
                        MapObjectId spawnType = MapObjectId::Chaser;
                        if (currentWave == 5) {
                            spawnType = MapObjectId::Boss;
                        } else if (rangeEnemiesToSpawn > 0) {
                            spawnType = MapObjectId::Range;
                        } else if (diverEnemiesToSpawn > 0) {
                            spawnType = MapObjectId::Diver;
                        } else if (currentWave >= 2 && currentWave <= 4 &&
                                   enemiesToSpawn == currentWave) {
                            // Add one ranged enemy at the start of waves 2-4.
                            spawnType = MapObjectId::Range;
                        } else if (currentWave >= 3 && currentWave <= 4 &&
                                   enemiesToSpawn == currentWave - 1) {
                            // Add one Diver after the ranged enemy in waves 3-4.
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
            }
        } else if (activeEnemies == 0) {
            if (currentWave == 5) {
                // Boss defeated!
                GameManager::GetInstance().SetState(GameState::VICTORY);
            } else {
                // Wave cleared! Setup next wave.
                currentWave++;
                if (currentWave == 5) {
                    enemiesToSpawn = 1; // Only 1 Boss
                } else {
                    enemiesToSpawn = currentWave;
                }
                timeBetweenWaves = 3.0f;
                showWaveTextTimer = 3.0f; // Give 3s warning for next wave
            }
        }
    }
}

void WaveManager::DrawHUD() {
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
