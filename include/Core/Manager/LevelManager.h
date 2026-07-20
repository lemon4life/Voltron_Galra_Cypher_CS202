#pragma once
#include <vector>
#include <string>
#include "raylib.h"
#include "Core/LevelAccess.h"
#include "Entities/GameObject.h"
#include "Core/Manager/EnemyPathManager.h"

class TeamManager;

class LevelManager
    : public IEntityRemovalAccess,
      public IEnemyPathAccess,
      public ILevelLineOfSightQuery {
private:
    std::vector<GameObject*> levelEntities;
    float levelWidth;
    float levelHeight;
    int gridRows;
    int gridCols;
    std::vector<std::vector<int>> mapGridLayer1;
    std::vector<std::vector<int>> mapGridLayer2;
    Texture2D tileset;

    // Enemy pathfinding and deferred entity removal.
    void ProcessPendingRemovals();
    std::vector<GameObject*> pendingRemoval;
    EnemyPathManager enemyPathManager;

public:
    LevelManager();
    ~LevelManager();

    void LoadLevel(const std::string& filepath, TeamManager* teamManager);
    void DrawLevel();
    void UpdateLevel(float deltaTime);
    void ClearLevel();
    void AddEntity(GameObject* entity);
    bool IsValidSpawnLocation(const GameObject* entity) const;
    bool IsSolidCollision(Rectangle box) const;

    Vector2 WorldToTile(Vector2 worldPos) const;
    Vector2 TileToWorld(int tileX, int tileY) const;

    float GetLevelWidth() const { return levelWidth; }
    float GetLevelHeight() const { return levelHeight; }
    EnemyPathManager& GetEnemyPathManager() { return enemyPathManager; }
    LevelAccessBundle GetLevelAccessBundle() {
        return { this, this, this };
    }

    bool HasClearLineOfSight(
        Vector2 start,
        Vector2 end,
        float projectileRadius = 5.0f
    ) const override;
    void QueueRemoval(GameObject* entity) override;
    void BeginPathFinding(PathfindingEnemy* enemy) override;
    void EndPathFinding(PathfindingEnemy* enemy) override;

    const std::vector<GameObject*>& GetEntities() const { return levelEntities; }
};
