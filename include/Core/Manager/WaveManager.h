#pragma once
#include "raylib.h"
#include <vector>
#include "Core/DepthRenderItem.h"

class TeamManager;
class LevelManager;

enum class DungeonAnnouncement {
    None,
    EnemiesIncoming,
    WaveStarted,
    BossWarning,
    RoomCleared
};

class WaveManager {
private:
    int currentWave;
    int enemiesToSpawn;
    int rangeEnemiesToSpawn;
    int diverEnemiesToSpawn;
    int droneEnemiesToSpawn;
    int demonTHAEnemiesToSpawn;
    float spawnTimer;
    float timeBetweenWaves;
    float showWaveTextTimer;

    // Dungeon room wave state
    int dungeonTotalWaves;
    int dungeonCurrentWave;
    bool isBossRoom;
    DungeonAnnouncement dungeonAnnouncement;

    void UpdateDungeonRoom(float deltaTime, TeamManager* teamManager, LevelManager* levelManager);
    void ConfigureDungeonWave(int floorNumber, int waveNumber);
    void SpawnEnemy(
        float deltaTime,
        TeamManager* teamManager,
        LevelManager* levelManager
    );

public:
    WaveManager();
    void Update(float deltaTime, TeamManager* teamManager, LevelManager* levelManager);
    void Reset(
        int startingEnemies = 1,
        int startingRangeEnemies = 0,
        int startingDiverEnemies = 0,
        int startingDemonTHAEnemies = 0
    );
    void DrawHUD();
    int GetCurrentWave() const { return currentWave; }
    bool SkipCurrentRoom(
        TeamManager* teamManager,
        LevelManager* levelManager
    );
};
