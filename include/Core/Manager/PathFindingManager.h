#pragma once

#include "Core/LevelAccess.h"
#include "Core/World/ObjectId.h"
#include "raylib.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class Enemy;
class LevelManager;
class ObjectManager;
struct EnemyNavigationCacheStore;

struct EnemyPathProfilingStats {
    int flowFieldBuildsThisFrame = 0;
    int flowFieldBuildsLastSecond = 0;
    int flowFieldCacheHitsLastSecond = 0;
    int activeFlowFieldProfiles = 0;
    float averageFlowFieldExpandedCells = 0.0f;
    int maximumFlowFieldExpandedCells = 0;
    float averageFlowFieldMilliseconds = 0.0f;
    float maximumFlowFieldMilliseconds = 0.0f;
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

class PathFindingManager : public IEnemyPathAccess {
private:
    enum class NavigationMode {
        PlayerFlowField,
        ExplicitGoalAStar
    };

    struct PathRecord {
        bool hasTargetTile = false;
        int targetTileX = 0;
        int targetTileY = 0;
        std::uint64_t navigationRevision = 0;
        float pathAge = 0.0f;
        bool forceRepath = true;
        bool lastSearchFailed = false;
        NavigationMode mode = NavigationMode::PlayerFlowField;
        Vector2 explicitGoal = { 0.0f, 0.0f };
    };

    LevelManager& levelManager;
    ObjectManager& objectManager;
    std::vector<ObjectId> enemies;
    std::unordered_map<ObjectId, PathRecord> pathRecords;
    std::unique_ptr<EnemyNavigationCacheStore> navigationCacheStore;
    int nextEnemyIndex = 0;
    float searchCredits = 0.0f;
    float profilingTimer = 0.0f;
    int profilingFlowFieldBuilds = 0;
    int profilingFlowFieldCacheHits = 0;
    int profilingFlowExpandedTotal = 0;
    int profilingFlowExpandedMaximum = 0;
    float profilingFlowMillisecondsTotal = 0.0f;
    float profilingFlowMillisecondsMaximum = 0.0f;
    int profilingSearches = 0;
    int profilingReady = 0;
    int profilingUnreachable = 0;
    int profilingSearchLimit = 0;
    int profilingExpandedTotal = 0;
    int profilingExpandedMaximum = 0;
    float profilingMillisecondsTotal = 0.0f;
    float profilingMillisecondsMaximum = 0.0f;
    EnemyPathProfilingStats profilingStats;

    void AddEnemy(Enemy& enemy);
    void AddEnemyTo(Enemy& enemy, Vector2 worldGoal);
    void RemoveEnemy(Enemy& enemy);

public:
    PathFindingManager(
        LevelManager& levelManager,
        ObjectManager& objectManager
    );
    ~PathFindingManager();
    PathFindingManager(const PathFindingManager&) = delete;
    PathFindingManager& operator=(const PathFindingManager&) = delete;

    void Clear();

    void BeginPathFinding(Enemy& enemy) override;
    void BeginPathFindingTo(Enemy& enemy, Vector2 worldGoal) override;
    void EndPathFinding(Enemy& enemy) override;
    bool IsBlocked(Rectangle bounds) const override;
    Rectangle GetLevelBounds() const override;
    std::optional<Vector2> GetNextMoveTarget(Enemy& enemy) override;
    std::vector<Vector2> GetNavigableTileCentersWithin(
        const Enemy& enemy,
        Vector2 origin,
        float radius
    ) const override;
    Vector2 GetLocalDirection(
        Enemy& enemy,
        Vector2 desiredDirection
    ) override;

    void Update(float deltaTime);
    const EnemyPathProfilingStats& GetProfilingStats() const {
        return profilingStats;
    }
};
