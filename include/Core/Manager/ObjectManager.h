#pragma once

#include "Core/DepthRenderItem.h"
#include "Core/LevelAccess.h"
#include "Core/World/ObjectId.h"
#include "Core/World/WorldDefinition.h"
#include "raylib.h"

#include <array>
#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class EffectManager;
class Enemy;
class GameObject;
class LevelManager;
class Paladin;
class Pot;
class Projectile;
class Rover;
class TeamManager;

struct QuintessenceOrb {
    static constexpr std::size_t HISTORY_CAPACITY = 12;
    Vector2 position = { 0.0f, 0.0f };
    Vector2 velocity = { 0.0f, 0.0f };
    bool isAttracted = false;
    float age = 0.0f;
    std::array<Vector2, HISTORY_CAPACITY> positionHistory = {};
    std::size_t historyStart = 0;
    std::size_t historyCount = 0;

    void PushHistory(Vector2 point) {
        historyStart = (historyStart + HISTORY_CAPACITY - 1) %
            HISTORY_CAPACITY;
        positionHistory[historyStart] = point;
        historyCount = std::min(historyCount + 1, HISTORY_CAPACITY);
    }

    Vector2 HistoryAt(std::size_t index) const {
        return positionHistory[(historyStart + index) % HISTORY_CAPACITY];
    }
};

struct ObjectManagerMemoryStats {
    std::size_t enemies = 0;
    std::size_t enemyCapacity = 0;
    std::size_t projectiles = 0;
    std::size_t projectileCapacity = 0;
    std::size_t pickups = 0;
    std::size_t assists = 0;
    std::size_t interactables = 0;
    std::size_t orbs = 0;
    std::size_t orbCapacity = 0;
    std::size_t pendingAdditions = 0;
    std::size_t pendingRemovals = 0;
};

class ObjectManager : public IEntityRemovalAccess {
private:
    LevelManager* levelManager = nullptr;
    IEnemyPathAccess* pathFinding = nullptr;
    EffectManager* effectManager = nullptr;
    TeamManager* teamManager = nullptr;

    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<std::unique_ptr<Projectile>> projectiles;
    std::vector<std::unique_ptr<Pot>> pickups;
    std::vector<std::unique_ptr<Rover>> assists;
    std::vector<std::unique_ptr<GameObject>> interactables;
    std::vector<QuintessenceOrb> orbs;

    std::vector<Enemy*> enemyView;
    std::unordered_map<ObjectId, Enemy*> enemyIndex;
    std::unordered_map<std::uint64_t, std::vector<Enemy*>>
        enemySpatialBuckets;
    std::vector<Enemy*> projectileEnemyScratch;
    std::vector<GameObject*> interactableView;
    std::unordered_set<GameObject*> pendingRemoval;
    std::vector<std::unique_ptr<GameObject>> pendingAddition;
    std::unordered_set<ObjectId> finalizedDeaths;
    std::unordered_set<ObjectId> silentRemoval;
    std::function<void(float)> hitstopCallback;

    void RebuildViews();
    void RebuildEnemySpatialIndex();
    void RouteObject(std::unique_ptr<GameObject> object);
    void ProcessPendingAdditions();
    void ProcessPendingRemovals();
    void FinalizeEnemyDeath(Enemy& enemy);

public:
    ObjectManager();
    ~ObjectManager();

    ObjectManager(const ObjectManager&) = delete;
    ObjectManager& operator=(const ObjectManager&) = delete;

    void Configure(
        LevelManager& level,
        IEnemyPathAccess& paths,
        EffectManager& effects,
        TeamManager* team
    );
    void SetTeamManager(TeamManager* team) { teamManager = team; }
    void SetHitstopCallback(std::function<void(float)> callback) {
        hitstopCallback = std::move(callback);
    }

    void SpawnAll(const DynamicSpawnList& requests);
    GameObject* Spawn(MapObjectId type, Vector2 position);
    bool QueueSpawn(MapObjectId type, Vector2 position);
    bool QueueEnemySpawnSafely(
        MapObjectId type,
        Vector2 desiredPosition,
        Rectangle allowedRoomBounds,
        float correctionRadius
    );
    void AddObject(std::unique_ptr<GameObject> object);
    void QueueRemoval(GameObject* object) override;
    void DeleteAllEnemies();
    void ClearProjectiles();
    void ClearOrbs();
    void Clear();

    void UpdateEntities(float deltaTime);
    void UpdateProjectiles(float deltaTime);
    void UpdateAssists(float deltaTime);
    void UpdateOrbs(float deltaTime);
    void CommitPendingChanges();

    void AddProjectile(std::unique_ptr<Projectile> projectile);
    void AddRover(std::unique_ptr<Rover> rover);
    void SpawnQuintessenceOrb(Vector2 position);

    void AddDepthRenderItems(std::vector<DepthRenderItem>& items);
    void DrawOrbs() const;
    void DrawDebugOverlays() const;

    const std::vector<Enemy*>& GetEnemies() const { return enemyView; }
    const std::vector<GameObject*>& GetInteractables() const {
        return interactableView;
    }
    std::size_t GetEnemyCount() const { return enemies.size(); }
    ObjectManagerMemoryStats GetMemoryStats() const;
    GameObject* FindObject(ObjectId id) const;
    Enemy* FindEnemy(ObjectId id) const;
    void GetEnemiesNear(Rectangle bounds, std::vector<Enemy*>& output) const;
    Pot* FindNearestPickup(Vector2 position, float radius) const;
    bool IsDynamicCollisionBlocked(
        Rectangle bounds,
        const GameObject* ignored = nullptr
    ) const;
};
