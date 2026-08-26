#include "Core/Manager/WaveManager.h"
#include "UI/UIUtils.h"
#include "Core/Manager/LevelManager.h"
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

namespace {
    void PushPlayerClearOfRoomGate(
        TeamManager* teamManager,
        LevelManager* levelManager
    ) {
        if (!teamManager || !levelManager) return;
        Paladin* player = teamManager->GetActivePaladin();
        if (!player) return;

        Vector2 escapePosition;
        if (levelManager->FindGateEscapePosition(
                player->GetCollisionBox(),
                player->GetPosition(),
                escapePosition)) {
            player->SetPosition(escapePosition);
        }
    }
}

WaveManager::WaveManager() {
    Reset();
}

void WaveManager::Reset(
    int startingEnemies,
    int startingRangeEnemies,
    int startingDiverEnemies,
    int startingDemonTHAEnemies
) {
    currentWave = startingEnemies == 0 ? 0 : 1;
    enemiesToSpawn = startingEnemies;
    rangeEnemiesToSpawn = std::clamp(startingRangeEnemies, 0, startingEnemies);
    diverEnemiesToSpawn = std::clamp(
        startingDiverEnemies,
        0,
        startingEnemies - rangeEnemiesToSpawn
    );
    demonTHAEnemiesToSpawn = std::clamp(
        startingDemonTHAEnemies,
        0,
        startingEnemies - rangeEnemiesToSpawn - diverEnemiesToSpawn
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
    activeEnemies = static_cast<int>(
        GameManager::GetInstance().GetObjectManager().GetEnemyCount()
    );

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

    // Room just locked — initialize waves if we haven't yet
    if (dungeonTotalWaves == 0) {
        // Strict guard check: Verify the locked room is a valid combat room (BATTLE or BOSS)
        std::shared_ptr<RoomNode> lockedNode = nullptr;
        for (const auto& node : levelManager->GetLevelMap().generatedNodes) {
            if (node->state == RoomState::LOCKED) {
                lockedNode = node;
                break;
            }
        }

        // If the locked room is an EVENT, EXIT, SPAWN, or small utility room, do NOT spawn combat waves
        if (!lockedNode || lockedNode->type == RoomType::EVENT ||
            lockedNode->type == RoomType::EXIT || lockedNode->type == RoomType::SPAWN ||
            (lockedNode->roomSize == 15 && lockedNode->type != RoomType::BOSS)) {
            return;
        }

        isBossRoom = (lockedNode->type == RoomType::BOSS);

        if (isBossRoom) {
            // Boss room: 1 wave with 1 boss
            dungeonTotalWaves = 1;
            dungeonCurrentWave = 1;
            enemiesToSpawn = 1;
            rangeEnemiesToSpawn = 0;
            diverEnemiesToSpawn = 0;
            demonTHAEnemiesToSpawn = 0;
            AudioManager::GetInstance().PlayMusicTrack("bgm_boss_theme", 1.5f);
        } else {
            // Normal battle room: 3 waves of progressive composition
            dungeonTotalWaves = 3;
            dungeonCurrentWave = 1;
            // Wave 1: 2-3 enemies (Chasers and Drones, with a chance of 1 Range)
            enemiesToSpawn = 2 + (rand() % 2); // 2-3
            rangeEnemiesToSpawn = (enemiesToSpawn >= 3) ? 1 : 0;
            diverEnemiesToSpawn = 0;
            demonTHAEnemiesToSpawn = 0;
        }

        currentWave = 1;
        timeBetweenWaves = 1.5f;
        showWaveTextTimer = 2.0f;
        return;
    }

    // Count active enemies
    int activeEnemies = 0;
    activeEnemies = static_cast<int>(
        GameManager::GetInstance().GetObjectManager().GetEnemyCount()
    );

    if (showWaveTextTimer > 0.0f) {
        showWaveTextTimer -= deltaTime;
    }

    if (timeBetweenWaves > 0.0f) {
        timeBetweenWaves -= deltaTime;
        if (timeBetweenWaves <= 0.0f) {
            timeBetweenWaves = 0.0f;
            PushPlayerClearOfRoomGate(teamManager, levelManager);
        }
    } else {
        if (enemiesToSpawn > 0) {
            SpawnEnemy(deltaTime, teamManager, levelManager);
        } else if (activeEnemies == 0) {
            // Wave cleared
            if (dungeonCurrentWave >= dungeonTotalWaves) {
                // All waves done — room cleared!
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
                    rangeEnemiesToSpawn = 0;
                    diverEnemiesToSpawn = 0;
                    demonTHAEnemiesToSpawn = 0;
                } else {
                    if (dungeonCurrentWave == 2) {
                        // Wave 2: 3-4 enemies (mix of Chaser/Drone, 1 Range, 1 Diver, optional 1 DemonTHA)
                        enemiesToSpawn = 3 + (rand() % 2); // 3-4
                        rangeEnemiesToSpawn = 1;
                        diverEnemiesToSpawn = 1;
                        demonTHAEnemiesToSpawn = (rand() % 2 == 0) ? 1 : 0;
                    } else {
                        // Wave 3: 4-5 enemies (heavy combat with DemonTHA, Range, Divers, and Grunts)
                        enemiesToSpawn = 4 + (rand() % 2); // 4-5
                        rangeEnemiesToSpawn = 1;
                        diverEnemiesToSpawn = 1;
                        demonTHAEnemiesToSpawn = 1 + (rand() % 2); // 1-2 DemonTHA
                    }
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

    Vector2 spawnPos;
    if (!levelManager->GetGuaranteedSpawnPoint(spawnPos)) {
        // No spots left in the room at all
        if (demonTHAEnemiesToSpawn > 0) demonTHAEnemiesToSpawn--;
        else if (diverEnemiesToSpawn > 0) diverEnemiesToSpawn--;
        else if (rangeEnemiesToSpawn > 0) rangeEnemiesToSpawn--;
        enemiesToSpawn--;
        spawnTimer = 0.07f;
        return;
    }

    MapObjectId spawnType = (GetRandomValue(0, 1) == 0) ? MapObjectId::Chaser : MapObjectId::Drone;

    if (isBossRoom ||
        (!levelManager->IsProceduralDungeon() && currentWave == 5)) {
        spawnType = MapObjectId::Boss;
    } else if (demonTHAEnemiesToSpawn > 0) {
        spawnType = MapObjectId::DemonTHA;
    } else if (diverEnemiesToSpawn > 0) {
        spawnType = MapObjectId::Diver;
    } else if (rangeEnemiesToSpawn > 0) {
        spawnType = MapObjectId::Range;
    } else if (!levelManager->IsProceduralDungeon()) {
        if (currentWave >= 4 && enemiesToSpawn == currentWave) {
            spawnType = MapObjectId::DemonTHA;
        } else if (currentWave >= 3 && enemiesToSpawn == currentWave - 1) {
            spawnType = MapObjectId::Diver;
        } else if (currentWave >= 2 && enemiesToSpawn == currentWave - 2) {
            spawnType = MapObjectId::Range;
        }
    }

    GameObject* newEnemy = GameManager::GetInstance()
        .GetObjectManager()
        .Spawn(spawnType, spawnPos);
    if (newEnemy) {
        // Apply Roguelike scaling buff
        int currentFloor = GameManager::GetInstance().GetCurrentFloor();
        float floorMultiplier = 1.0f + ((currentFloor - 1) * 0.3f);
        if (spawnType != MapObjectId::Boss) {
            static_cast<Enemy*>(newEnemy)->ApplyStatMultiplier(
                floorMultiplier
            );
        }

        if (spawnType == MapObjectId::DemonTHA && demonTHAEnemiesToSpawn > 0) {
            demonTHAEnemiesToSpawn--;
        } else if (spawnType == MapObjectId::Diver && diverEnemiesToSpawn > 0) {
            diverEnemiesToSpawn--;
        } else if (spawnType == MapObjectId::Range && rangeEnemiesToSpawn > 0) {
            rangeEnemiesToSpawn--;
        }
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
