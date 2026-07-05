#include "Core/WaveManager.h"
#include "Core/LevelManager.h"
#include "Core/EntityFactory.h"
#include "Entities/Player.h"
#include "Entities/Enemy.h"
#include "raymath.h"
#include <cstdlib>

WaveManager::WaveManager() {
    Reset();
}

void WaveManager::Reset() {
    currentWave = 1;
    enemiesToSpawn = 1;
    spawnTimer = 0.0f;
    timeBetweenWaves = 3.0f;
    showWaveTextTimer = 2.0f;
}

void WaveManager::Update(float deltaTime, Player* player, LevelManager* levelManager) {
    // Determine active enemies dynamically
    int activeEnemies = 0;
    for (auto* entity : levelManager->GetEntities()) {
        if (dynamic_cast<Enemy*>(entity)) {
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

                    if (Vector2Distance(spawnPos, player->GetPosition()) > 150.0f) {
                        if (levelManager->IsValidSpawnLocation(spawnPos)) {
                            GameObject* newEnemy = EntityFactory::CreateEntity('C', spawnPos, player);
                            if (newEnemy) {
                                levelManager->AddEntity(newEnemy);
                                enemiesToSpawn--;
                                spawnTimer = 0.5f;
                                spawned = true;
                            }
                        }
                    }
                    attempts++;
                }
            }
        } else if (activeEnemies == 0) {
            // Wave cleared! Setup next wave.
            currentWave++;
            enemiesToSpawn = currentWave * 3;
            timeBetweenWaves = 3.0f;
            showWaveTextTimer = 2.0f;
        }
    }
}

void WaveManager::DrawHUD() {
    DrawText(TextFormat("WAVE: %d", currentWave), 390, 10, 20, WHITE);
    if (showWaveTextTimer > 0.0f) {
        if (currentWave == 1) {
            DrawText("WAVE 1 START!", 180, 240, 20, GREEN);
        } else {
            DrawText("WAVE CLEARED!", 180, 240, 20, GREEN);
        }
    }
}
