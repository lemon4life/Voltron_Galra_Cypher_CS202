#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "raylib.h"
#include "Core/LevelAccess.h"
#include "Entities/GameObject.h"
#include "Core/Manager/EnemyPathManager.h"
#include "Core/Level/Tilemap.h"
#include "Core/DepthRenderItem.h"

class TeamManager;

class LevelManager
    : public IEntityRemovalAccess,
      public IEnemyPathAccess,
      public ILevelLineOfSightQuery,
      public IMapObjectDestroyAccess {
private:
    enum class LevelMode {
        Layered,
        Procedural
    };

    struct PendingMapObjectDestruction {
        GameObject* object;
        GameObjectCell cell;
    };

    std::vector<GameObject*> levelEntities;
    float levelWidth;
    float levelHeight;
    int gridRows;
    int gridCols;
    std::vector<std::vector<int>> mapGridLayer1;
    std::vector<std::vector<int>> mapGridLayer2;
    std::vector<std::vector<MapObjectId>> mapObjectGrid;
    Texture2D floorTileset;
    Texture2D wallTileset;
    Texture2D boxTexture;
    Texture2D gateTexture;
    Texture2D prop1Texture;
    Texture2D prop2Texture;
    Texture2D tileset; // legacy from remote
    bool useLegacyMap = true;
    LevelMode levelMode = LevelMode::Layered;
    std::shared_ptr<RoomTemplate> activeRoom;
    LevelMap levelMap;
    std::vector<Rectangle> currentRoomWalls;
    std::vector<Rectangle> doorColliders;
    std::shared_ptr<RoomNode> currentlyLockedRoom;
    Vector2 roomOffset = {0.0f, 0.0f};
    Vector2 nudgePosition = {0.0f, 0.0f};
    bool needsNudge = false;
    std::uint64_t navigationRevision = 0;


    bool LoadObjectGrid(const std::string& filepath);
    void SpawnGameObjects(TeamManager* teamManager);
    bool IsSolidMapObject(MapObjectId objectId) const;
    void MarkNavigationChanged() { ++navigationRevision; }

    // Enemy pathfinding and deferred object/entity removal.
    void ProcessPendingMapObjectDestructions();
    void ProcessPendingAdditions();
    void ProcessPendingRemovals();
    std::vector<PendingMapObjectDestruction> pendingMapObjectDestructions;
    std::vector<GameObject*> pendingRemoval;
    std::vector<GameObject*> pendingAddition;
    EnemyPathManager enemyPathManager;

public:
    LevelManager();
    ~LevelManager();

    void LoadLevel(const std::string& filepath, TeamManager* teamManager);

    const LevelMap& GetLevelMap() const { return levelMap; }
    RoomState GetActiveRoomState() const { return (currentlyLockedRoom) ? currentlyLockedRoom->state : RoomState::IDLE; }
    void SetActiveRoomState(RoomState s) {
        if (currentlyLockedRoom && currentlyLockedRoom->state != s) {
            currentlyLockedRoom->state = s;
            MarkNavigationChanged();
        }
    }
    bool IsProceduralDungeon() const {
        return levelMode == LevelMode::Procedural;
    }
    bool NeedsPlayerNudge() const { return needsNudge; }
    Vector2 ConsumeNudge() { needsNudge = false; return nudgePosition; }
    bool IsPlayerInExitRoom(Vector2 playerPos) const;
    void GenerateDungeon(TeamManager* teamManager);

    void DrawLevelBase();
    void GetDepthRenderItems(std::vector<DepthRenderItem>& items);
    void UpdateLevel(float deltaTime, Vector2 playerPos = {0,0});
    void ClearLevel();
    void AddEntity(GameObject* entity);
    bool QueueEnemySpawn(
        MapObjectId enemyType,
        Vector2 position,
        TeamManager* teamManager
    );
    bool IsValidSpawnLocation(const GameObject* entity) const;
    bool IsSolidCollision(Rectangle box) const;

    Vector2 WorldToTile(Vector2 worldPos) const;
    Vector2 TileToWorld(int tileX, int tileY) const;

    float GetLevelWidth() const { return levelWidth; }
    float GetLevelHeight() const { return levelHeight; }
    LevelAccessBundle GetLevelAccessBundle() {
        return { *this, *this, *this, *this };
    }

    bool HasClearLineOfSight(
        Vector2 start,
        Vector2 end,
        float projectileRadius = 5.0f
    ) const override;
    void QueueRemoval(GameObject* entity) override;
    void QueueMapObjectDestruction(
        GameObject& object,
        GameObjectCell cell
    ) override;
    void BeginPathFinding(Enemy& enemy) override;
    void EndPathFinding(Enemy& enemy) override;
    bool IsBlocked(Rectangle bounds) const override;
    Rectangle GetLevelBounds() const override;
    std::optional<Vector2> GetNextMoveTarget(
        Enemy& enemy
    ) override;
    Rectangle GetCurrentRoomBounds() const;
    std::uint64_t GetNavigationRevision() const {
        return navigationRevision;
    }
    const EnemyPathProfilingStats& GetEnemyPathProfilingStats() const {
        return enemyPathManager.GetProfilingStats();
    }
    Vector2 GetLocalDirection(
        Enemy& enemy,
        Vector2 desiredDirection
    ) override;

    const std::vector<GameObject*>& GetEntities() const { return levelEntities; }
};
