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

    /// Runs the locked-room wave lifecycle from its opening delay through completion.
    /// It configures enemy composition, schedules spawns, and unlocks cleared rooms.
    void UpdateDungeonRoom(float deltaTime, TeamManager* teamManager, LevelManager* levelManager);
    /// Configures dungeon wave.
    void ConfigureDungeonWave(int floorNumber, int waveNumber);
    /// Creates the requested enemy with current floor modifiers and queues it for safe insertion.
    void SpawnEnemy(
        float deltaTime,
        TeamManager* teamManager,
        LevelManager* levelManager
    );

public:
    /// Creates a WaveManager instance from the supplied configuration.
    WaveManager();
    /// Advances this component's state for the current frame.
    void Update(float deltaTime, TeamManager* teamManager, LevelManager* levelManager);
    /// Restores this component to its initial runtime state.
    void Reset(
        int startingEnemies = 1,
        int startingRangeEnemies = 0,
        int startingDiverEnemies = 0,
        int startingDemonTHAEnemies = 0
    );
    /// Renders hud.
    void DrawHUD();
    /// Returns the current current wave.
    int GetCurrentWave() const { return currentWave; }
    /// Implements the skip current room behavior for this component.
    bool SkipCurrentRoom(
        TeamManager* teamManager,
        LevelManager* levelManager
    );
};
