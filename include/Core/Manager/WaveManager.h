#pragma once
#include "raylib.h"

class TeamManager;
class LevelManager;

class WaveManager {
private:
    int currentWave;
    int enemiesToSpawn;
    int rangeEnemiesToSpawn;
    int diverEnemiesToSpawn;
    float spawnTimer;
    float timeBetweenWaves;
    float showWaveTextTimer;

    // Dungeon room wave state
    int dungeonTotalWaves;
    int dungeonCurrentWave;
    bool isBossRoom;

    void UpdateDungeonRoom(float deltaTime, TeamManager* teamManager, LevelManager* levelManager);
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
        int startingDiverEnemies = 0
    );
    void DrawHUD();
    int GetCurrentWave() const { return currentWave; }
    void StartRoomWaves(int totalEnemies);
    bool IsRoomCleared() const;
};
