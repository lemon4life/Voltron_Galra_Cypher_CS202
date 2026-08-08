#pragma once
#include "raylib.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class LevelManager;
class Enemy;
struct EnemyNavigationCacheStore;

struct EnemyPathProfilingStats {
    int searchesThisFrame = 0;
    int searchesLastSecond = 0;
    int readyLastSecond = 0;
    int unreachableLastSecond = 0;
    int searchLimitLastSecond = 0;
    float averageExpandedCells = 0.0f;
    int maximumExpandedCells = 0;
    float averageSearchMilliseconds = 0.0f;
    float maximumSearchMilliseconds = 0.0f;
};

class EnemyPathManager {
private:
    struct PathRecord {
        bool hasTargetTile = false;
        int targetTileX = 0;
        int targetTileY = 0;
        std::uint64_t navigationRevision = 0;
        float pathAge = 0.0f;
        bool forceRepath = true;
        bool lastSearchFailed = false;
    };

    std::vector<Enemy*> enemies;
    std::unordered_map<Enemy*, PathRecord> pathRecords;
    std::unique_ptr<EnemyNavigationCacheStore> navigationCacheStore;
    int nextEnemyIndex = 0;
    float searchCredits = 0.0f;
    float profilingTimer = 0.0f;
    int profilingSearches = 0;
    int profilingReady = 0;
    int profilingUnreachable = 0;
    int profilingSearchLimit = 0;
    int profilingExpandedTotal = 0;
    int profilingExpandedMaximum = 0;
    float profilingMillisecondsTotal = 0.0f;
    float profilingMillisecondsMaximum = 0.0f;
    EnemyPathProfilingStats profilingStats;

public:
    EnemyPathManager();
    ~EnemyPathManager();
    EnemyPathManager(const EnemyPathManager&) = delete;
    EnemyPathManager& operator=(const EnemyPathManager&) = delete;

    void AddEnemy(Enemy& enemy);

    void RemoveEnemy(Enemy& enemy);
    void Clear();

    std::optional<Vector2> GetNextMoveTarget(
        LevelManager& levelManager,
        Enemy& enemy
    );

    Vector2 GetLocalAvoidanceDirection(
        LevelManager& levelManager,
        Enemy& enemy,
        Vector2 desiredDirection
    );

    void Update(LevelManager& levelManager, float deltaTime);
    const EnemyPathProfilingStats& GetProfilingStats() const {
        return profilingStats;
    }
};
