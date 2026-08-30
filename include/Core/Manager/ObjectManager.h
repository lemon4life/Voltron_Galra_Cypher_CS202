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

    /// Adds a runtime count sample to the fixed-length diagnostics history.
    void PushHistory(Vector2 point) {
        historyStart = (historyStart + HISTORY_CAPACITY - 1) %
            HISTORY_CAPACITY;
        positionHistory[historyStart] = point;
        historyCount = std::min(historyCount + 1, HISTORY_CAPACITY);
    }

    /// Returns a diagnostics history sample by logical age rather than storage index.
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

// Design Patterns - Deferred Mutation Queue and RAII Ownership:
// ObjectManager owns entities/projectiles with unique_ptr. Producers queue work
// in pendingAddition/pendingRemoval; CommitPendingChanges is the mutation barrier
// that safely updates owning containers, raw-pointer views, and spatial indexes.
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

    /// Rebuilds views.
    void RebuildViews();
    /// Rebuilds enemy spatial index.
    void RebuildEnemySpatialIndex();
    /// Routes object.
    void RouteObject(std::unique_ptr<GameObject> object);
    /// Processes pending additions.
    void ProcessPendingAdditions();
    /// Processes pending removals.
    void ProcessPendingRemovals();
    /// Completes enemy death rewards and removes the dead enemy from managed views.
    void FinalizeEnemyDeath(Enemy& enemy);

public:
    /// Creates a ObjectManager instance from the supplied configuration.
    ObjectManager();
    /// Releases resources owned by this ObjectManager instance.
    ~ObjectManager();

    /// Creates a ObjectManager instance from the supplied configuration.
    ObjectManager(const ObjectManager&) = delete;
    ObjectManager& operator=(const ObjectManager&) = delete;

    /// Connects this component to the managers and services it needs at runtime.
    void Configure(
        LevelManager& level,
        IEnemyPathAccess& paths,
        EffectManager& effects,
        TeamManager* team
    );
    /// Updates the stored team manager.
    void SetTeamManager(TeamManager* team) { teamManager = team; }
    /// Updates the stored hitstop callback.
    void SetHitstopCallback(std::function<void(float)> callback) {
        hitstopCallback = std::move(callback);
    }

    /// Spawns all.
    void SpawnAll(const DynamicSpawnList& requests);
    /// Creates and queues the requested runtime entity without mutating active iteration.
    GameObject* Spawn(MapObjectId type, Vector2 position);
    /// Queues spawn.
    bool QueueSpawn(MapObjectId type, Vector2 position);
    /// Queues enemy spawn safely.
    bool QueueEnemySpawnSafely(
        MapObjectId type,
        Vector2 desiredPosition,
        Rectangle allowedRoomBounds,
        float correctionRadius,
        const std::function<void(Enemy&)>& configureEnemy = {}
    );
    /// Adds object.
    void AddObject(std::unique_ptr<GameObject> object);
    /// Queues removal.
    void QueueRemoval(GameObject* object) override;
    /// Deletes all enemies.
    void DeleteAllEnemies();
    /// Clears projectiles.
    void ClearProjectiles();
    /// Clears orbs.
    void ClearOrbs();
    /// Removes all runtime entries owned by this component and resets transient state.
    void Clear();

    /// Updates entities.
    void UpdateEntities(float deltaTime);
    /// Advances projectiles, resolves their collisions, and removes inactive shots safely.
    void UpdateProjectiles(float deltaTime);
    /// Updates assists.
    void UpdateAssists(float deltaTime);
    /// Updates orbs.
    void UpdateOrbs(float deltaTime);
    /// Applies queued additions and removals after iteration is safe to modify.
    void CommitPendingChanges();

    /// Adds projectile.
    void AddProjectile(std::unique_ptr<Projectile> projectile);
    /// Adds rover.
    void AddRover(std::unique_ptr<Rover> rover);
    /// Spawns quintessence orb.
    void SpawnQuintessenceOrb(Vector2 position);

    /// Adds depth render items.
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items);
    /// Renders orbs.
    void DrawOrbs() const;
    /// Renders debug overlays.
    void DrawDebugOverlays() const;

    /// Returns the current enemies.
    const std::vector<Enemy*>& GetEnemies() const { return enemyView; }
    /// Returns the current interactables.
    const std::vector<GameObject*>& GetInteractables() const {
        return interactableView;
    }
    /// Returns the current enemy count.
    std::size_t GetEnemyCount() const { return enemies.size(); }
    /// Returns the current memory stats.
    ObjectManagerMemoryStats GetMemoryStats() const;
    /// Searches for object.
    GameObject* FindObject(ObjectId id) const;
    /// Searches for enemy.
    Enemy* FindEnemy(ObjectId id) const;
    /// Returns the current enemies near.
    void GetEnemiesNear(Rectangle bounds, std::vector<Enemy*>& output) const;
    /// Searches for nearest pickup.
    Pot* FindNearestPickup(Vector2 position, float radius) const;
    /// Reports whether the dynamic collision blocked condition is satisfied.
    bool IsDynamicCollisionBlocked(
        Rectangle bounds,
        const GameObject* ignored = nullptr
    ) const;
};
