#pragma once
#include "raylib.h"

class TeamManager;
class LevelManager;

class WaveManager {
private:
    int currentWave;
    int enemiesToSpawn;
    float spawnTimer;
    float timeBetweenWaves;
    float showWaveTextTimer;

public:
    WaveManager();
    void Update(float deltaTime, TeamManager* teamManager, LevelManager* levelManager);
    void Reset(int startingEnemies = 1);
    void DrawHUD();
};
