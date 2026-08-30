#include "Core/Manager/WaveManager.h"
#include "UI/UIUtils.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Diagnostics/MemoryDiagnostics.h"
#include "Core/Manager/ObjectManager.h"
#include "Entities/Props/Chest.h"
#include "Core/Level/RoomNode.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
    struct FloorCombatProfile {
        float healthMultiplier;
        float damageMultiplier;
        float speedMultiplier;
        float waveSizeMultiplier;
    };

    FloorCombatProfile GetFloorCombatProfile(int floorNumber) {
        switch (std::clamp(floorNumber, 1, 5)) {
            case 2:
                return { 1.0f, 1.20f, 1.0f, 1.0f };
            case 3:
                return { 1.25f, 1.20f, 1.0f, 1.20f };
            case 4:
                return { 1.30f, 1.50f, 1.20f, 1.50f };
            case 5:
                return { 1.50f, 1.50f, 1.20f, 1.50f };
            case 1:
            default:
                return { 1.0f, 1.0f, 1.0f, 1.0f };
        }
    }

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
    droneEnemiesToSpawn = 0;
    demonTHAEnemiesToSpawn = std::clamp(
        startingDemonTHAEnemies,
        0,
        startingEnemies - rangeEnemiesToSpawn - diverEnemiesToSpawn
    );
    spawnTimer = 0.0f;
    timeBetweenWaves = 3.0f;
    showWaveTextTimer = startingEnemies > 0 ? 2.0f : 0.0f;

    // Dungeon room state
    dungeonTotalWaves = 0;
    dungeonCurrentWave = 0;
    isBossRoom = false;
    dungeonAnnouncement = DungeonAnnouncement::None;
}

void WaveManager::ConfigureDungeonWave(
    int floorNumber,
    int waveNumber
) {
    int baseEnemyCount = 2 + (rand() % 2);
    if (waveNumber == 2) {
        baseEnemyCount = 3 + (rand() % 2);
    } else if (waveNumber >= 3) {
        baseEnemyCount = 4 + (rand() % 2);
    }

    FloorCombatProfile profile = GetFloorCombatProfile(floorNumber);
    enemiesToSpawn = std::max(
        1,
        (int)std::ceil(baseEnemyCount * profile.waveSizeMultiplier)
    );

    rangeEnemiesToSpawn = 1;
    diverEnemiesToSpawn = floorNumber >= 2 && waveNumber >= 2 ? 1 : 0;
    droneEnemiesToSpawn = floorNumber >= 3 && waveNumber >= 2 ? 1 : 0;
    demonTHAEnemiesToSpawn = floorNumber >= 4 && waveNumber >= 2
        ? 1
        : 0;

    // Preserve the total count even if this policy is tuned to smaller waves.
    int reservedCount = rangeEnemiesToSpawn + diverEnemiesToSpawn +
        droneEnemiesToSpawn + demonTHAEnemiesToSpawn;
    while (reservedCount > enemiesToSpawn) {
        if (demonTHAEnemiesToSpawn > 0) {
            --demonTHAEnemiesToSpawn;
        } else if (droneEnemiesToSpawn > 0) {
            --droneEnemiesToSpawn;
        } else if (diverEnemiesToSpawn > 0) {
            --diverEnemiesToSpawn;
        } else {
            --rangeEnemiesToSpawn;
        }
        --reservedCount;
    }
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
                MemoryDiagnostics::Capture(
                    "static_wave_completed",
                    GameManager::GetInstance()
                );
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
        if (showWaveTextTimer > 0.0f) {
            showWaveTextTimer -= deltaTime;
            if (showWaveTextTimer <= 0.0f) {
                showWaveTextTimer = 0.0f;
                dungeonAnnouncement = DungeonAnnouncement::None;
            }
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

        // Only explicit battle and boss rooms may start combat. Room size is
        // not a room-type signal because normal battles can now be 15x15.
        if (!lockedNode ||
            (lockedNode->type != RoomType::BATTLE &&
             lockedNode->type != RoomType::BOSS)) {
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
            droneEnemiesToSpawn = 0;
            demonTHAEnemiesToSpawn = 0;
            AudioManager::GetInstance().PlayMusicTrack("bgm_boss_theme", 1.5f);
        } else {
            // Normal battle room: 3 waves of progressive composition
            dungeonTotalWaves = 3;
            dungeonCurrentWave = 1;
            ConfigureDungeonWave(
                GameManager::GetInstance().GetCurrentFloor(),
                dungeonCurrentWave
            );
        }

        currentWave = 1;
        timeBetweenWaves = 1.5f;
        showWaveTextTimer = 2.0f;
        dungeonAnnouncement = isBossRoom
            ? DungeonAnnouncement::BossWarning
            : DungeonAnnouncement::EnemiesIncoming;
        MemoryDiagnostics::Capture(
            isBossRoom ? "boss_room_started" : "battle_room_started",
            GameManager::GetInstance()
        );
        return;
    }

    // Count active enemies
    int activeEnemies = 0;
    activeEnemies = static_cast<int>(
        GameManager::GetInstance().GetObjectManager().GetEnemyCount()
    );

    if (showWaveTextTimer > 0.0f) {
        showWaveTextTimer -= deltaTime;
        if (showWaveTextTimer <= 0.0f) {
            showWaveTextTimer = 0.0f;
            dungeonAnnouncement = DungeonAnnouncement::None;
        }
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

                // Spawn reward chest at active Paladin's position with teleport VFX
                if (teamManager && teamManager->GetActivePaladin()) {
                    Vector2 spawnPos = teamManager->GetActivePaladin()->GetPosition();
                    Texture2D smoke = AssetManager::GetInstance().GetTexture("AppearSmoke");
                    Texture2D light = AssetManager::GetInstance().GetTexture("AppearLight");
                    GameManager::GetInstance().AddEffect(spawnPos, smoke, 5, 0.5f);
                    GameManager::GetInstance().AddEffect(spawnPos, light, 5, 0.5f);
                    AudioManager::GetInstance().PlaySoundEffect("fx_show_up");

                    GameManager::GetInstance().GetObjectManager().AddObject(
                        std::make_unique<Chest>(spawnPos, ChestRewardType::Coins)
                    );
                }

                // Reset for next room
                dungeonTotalWaves = 0;
                dungeonCurrentWave = 0;
                currentWave = 0;
                isBossRoom = false;
                showWaveTextTimer = 1.5f;
                dungeonAnnouncement = DungeonAnnouncement::RoomCleared;
                MemoryDiagnostics::Capture(
                    "battle_room_completed",
                    GameManager::GetInstance()
                );
            } else {
                // Next wave
                dungeonCurrentWave++;
                currentWave = dungeonCurrentWave;

                if (isBossRoom) {
                    enemiesToSpawn = 1;
                    rangeEnemiesToSpawn = 0;
                    diverEnemiesToSpawn = 0;
                    droneEnemiesToSpawn = 0;
                    demonTHAEnemiesToSpawn = 0;
                } else {
                    ConfigureDungeonWave(
                        GameManager::GetInstance().GetCurrentFloor(),
                        dungeonCurrentWave
                    );
                }

                timeBetweenWaves = 2.0f;
                showWaveTextTimer = 2.0f;
                dungeonAnnouncement = DungeonAnnouncement::WaveStarted;
                MemoryDiagnostics::Capture(
                    "dungeon_wave_completed",
                    GameManager::GetInstance()
                );
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

    Vector2 spawnPos = { 0.0f, 0.0f };
    std::shared_ptr<RoomNode> lockedRoom =
        levelManager->GetCurrentlyLockedRoom();
    bool hasSpawnPosition = false;
    if (isBossRoom && lockedRoom) {
        Rectangle bossRoomBounds = lockedRoom->GetWorldBounds();
        spawnPos = {
            bossRoomBounds.x + bossRoomBounds.width * 0.5f,
            bossRoomBounds.y + bossRoomBounds.height * 0.5f
        };
        hasSpawnPosition = true;
    } else {
        hasSpawnPosition = levelManager->GetGuaranteedSpawnPoint(spawnPos);
    }

    if (!hasSpawnPosition) {
        // No spots left in the room at all
        if (demonTHAEnemiesToSpawn > 0) demonTHAEnemiesToSpawn--;
        else if (droneEnemiesToSpawn > 0) droneEnemiesToSpawn--;
        else if (diverEnemiesToSpawn > 0) diverEnemiesToSpawn--;
        else if (rangeEnemiesToSpawn > 0) rangeEnemiesToSpawn--;
        enemiesToSpawn--;
        spawnTimer = 0.07f;
        return;
    }

    MapObjectId spawnType = MapObjectId::Chaser;
    if (!levelManager->IsProceduralDungeon()) {
        spawnType = (GetRandomValue(0, 1) == 0)
            ? MapObjectId::Chaser
            : MapObjectId::Drone;
    }

    if (isBossRoom ||
        (!levelManager->IsProceduralDungeon() && currentWave == 5)) {
        spawnType = MapObjectId::Boss;
    } else if (demonTHAEnemiesToSpawn > 0) {
        spawnType = MapObjectId::DemonTHA;
    } else if (droneEnemiesToSpawn > 0) {
        spawnType = MapObjectId::Drone;
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
    if (Enemy* enemy = dynamic_cast<Enemy*>(newEnemy)) {
        int currentFloor = GameManager::GetInstance().GetCurrentFloor();
        FloorCombatProfile profile = GetFloorCombatProfile(currentFloor);
        if (spawnType != MapObjectId::Boss) {
            enemy->ApplyStatMultipliers(
                profile.healthMultiplier,
                profile.damageMultiplier,
                profile.speedMultiplier
            );
        }

        if (spawnType == MapObjectId::DemonTHA && demonTHAEnemiesToSpawn > 0) {
            demonTHAEnemiesToSpawn--;
        } else if (spawnType == MapObjectId::Drone && droneEnemiesToSpawn > 0) {
            droneEnemiesToSpawn--;
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

        }

        if (showWaveTextTimer > 0.0f) {
            switch (dungeonAnnouncement) {
                case DungeonAnnouncement::EnemiesIncoming:
                    UIUtils::DrawText("PixeloidBold", "ENEMIES INCOMING!", { 200.0f, 240.0f }, static_cast<UIUtils::FontSize>(20), RED);
                    break;
                case DungeonAnnouncement::WaveStarted:
                    UIUtils::DrawText("PixeloidBold", TextFormat("WAVE %d!", dungeonCurrentWave), { 260.0f, 240.0f }, static_cast<UIUtils::FontSize>(20), YELLOW);
                    break;
                case DungeonAnnouncement::BossWarning:
                    UIUtils::DrawText("PixeloidBold", "BOSS WARNING!", { 200.0f, 240.0f }, static_cast<UIUtils::FontSize>(28), RED);
                    break;
                case DungeonAnnouncement::RoomCleared:
                    UIUtils::DrawText("PixeloidBold", "ROOM CLEARED!", { 240.0f, 240.0f }, static_cast<UIUtils::FontSize>(20), GREEN);
                    break;
                case DungeonAnnouncement::None:
                default:
                    break;
            }
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

bool WaveManager::SkipCurrentRoom(
    TeamManager* teamManager,
    LevelManager* levelManager
) {
    (void)teamManager;
    if (!levelManager || !levelManager->IsProceduralDungeon() ||
        levelManager->GetActiveRoomState() != RoomState::LOCKED) {
        return false;
    }

    GameManager& gameManager = GameManager::GetInstance();
    gameManager.GetObjectManager().DeleteAllEnemies();
    gameManager.GetObjectManager().CommitPendingChanges();
    gameManager.ClearProjectiles();
    levelManager->SetActiveRoomState(RoomState::CLEARED);

    if (isBossRoom) {
        AudioManager::GetInstance().PlayMusicTrack("bgm_battle", 1.0f);
    }

    enemiesToSpawn = 0;
    rangeEnemiesToSpawn = 0;
    diverEnemiesToSpawn = 0;
    droneEnemiesToSpawn = 0;
    demonTHAEnemiesToSpawn = 0;
    dungeonTotalWaves = 0;
    dungeonCurrentWave = 0;
    currentWave = 0;
    timeBetweenWaves = 0.0f;
    spawnTimer = 0.0f;
    isBossRoom = false;
    showWaveTextTimer = 1.5f;
    dungeonAnnouncement = DungeonAnnouncement::RoomCleared;
    MemoryDiagnostics::Capture("battle_room_skipped", gameManager);
    return true;
}
