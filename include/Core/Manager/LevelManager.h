#pragma once
#include <vector>
#include <string>
#include "raylib.h"
#include "Core/LevelAccess.h"
#include "Entities/GameObject.h"
#include "Core/Manager/EnemyPathManager.h"
#include "Core/Level/Tilemap.h"

class TeamManager;

class LevelManager
    : public IEntityRemovalAccess,
      public IEnemyPathAccess,
      public ILevelLineOfSightQuery,
      public IMapObjectDestroyAccess {
private:
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
    Texture2D tileset;

    bool useLegacyMap = true;
    std::shared_ptr<RoomTemplate> activeRoom;
    LevelMap levelMap;
    std::vector<Rectangle> currentRoomWalls;
    std::vector<Rectangle> doorColliders;
    std::shared_ptr<RoomNode> currentlyLockedRoom;
    Vector2 roomOffset = {0.0f, 0.0f};
    Vector2 nudgePosition = {0.0f, 0.0f};
    bool needsNudge = false;


    bool LoadObjectGrid(const std::string& filepath);
    void SpawnGameObjects();
    bool IsSolidMapObject(MapObjectId objectId) const;

    // Enemy pathfinding and deferred object/entity removal.
    void ProcessPendingMapObjectDestructions();
    void ProcessPendingRemovals();
    std::vector<PendingMapObjectDestruction> pendingMapObjectDestructions;
    std::vector<GameObject*> pendingRemoval;
    EnemyPathManager enemyPathManager;

public:
    LevelManager();
    ~LevelManager();

    void LoadLevel(const std::string& filepath, TeamManager* teamManager);
    
    void SetUseLegacyMap(bool legacy) { useLegacyMap = legacy; }
    const LevelMap& GetLevelMap() const { return levelMap; }
    RoomState GetActiveRoomState() const { return (currentlyLockedRoom) ? currentlyLockedRoom->state : RoomState::IDLE; }
    void SetActiveRoomState(RoomState s) { if(currentlyLockedRoom) currentlyLockedRoom->state = s; }
    bool IsLegacyMap() const { return useLegacyMap; }
    bool NeedsPlayerNudge() const { return needsNudge; }
    Vector2 ConsumeNudge() { needsNudge = false; return nudgePosition; }
    void GenerateDungeon(TeamManager* teamManager);

    void DrawLevel();
    void UpdateLevel(float deltaTime, Vector2 playerPos = {0,0});
    void ClearLevel();
    void AddEntity(GameObject* entity);
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
    Rectangle GetCurrentRoomBounds() const;
    Vector2 GetNextMoveTarget(
        Enemy& enemy,
        Vector2 fallbackTarget
    ) override;
    Vector2 GetLocalDirection(
        Enemy& enemy,
        Vector2 desiredDirection
    ) override;

    const std::vector<GameObject*>& GetEntities() const { return levelEntities; }
};
