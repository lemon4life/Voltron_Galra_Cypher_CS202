#pragma once
#include <vector>
#include <string>
#include "raylib.h"
#include "Entities/GameObject.h"
#include "Core/Manager/EnemyPathManager.h"
class Player;

class LevelManager : public IEnemyObserver {
private:
    std::vector<GameObject*> levelEntities;
    float levelWidth;
    float levelHeight;
    int gridRows;
    int gridCols;
    std::vector<std::vector<int>> mapGridLayer1;
    std::vector<std::vector<int>> mapGridLayer2;
    Texture2D tileset;

    // Data for Enemy Path Manger
    void ProcessPendingRemovals();
    std::vector<Enemy*> pendingRemoval;
    EnemyPathManager enemyPathManager;

public:
    LevelManager();
    ~LevelManager();

    void LoadLevel(const std::string& filepath, Player* player);
    void DrawLevel();
    void UpdateLevel(float deltaTime);
    void ClearLevel();
    void AddEntity(GameObject* entity);
    bool IsValidSpawnLocation(Vector2 position) const;
    bool IsValidSpawnLocation(const GameObject* entity) const;
    bool IsSolidCollision(Rectangle box) const;

    bool IsWalkableTile(int x, int y) const;
    Vector2 WorldToTile(Vector2 worldPos) const;
    Vector2 TileToWorld(int tileX, int tileY) const;

    float GetLevelWidth() const { return levelWidth; }
    float GetLevelHeight() const { return levelHeight; }

    // Override functions of Enemy Observer
    void OnEnemyPathFind(Enemy* enemy) override;
    void OnEnemyPathFindEnded(Enemy* enemy) override;
    void OnEnemyDied(Enemy* enemy) override;

    const std::vector<GameObject*>& GetEntities() const { return levelEntities; }
};
