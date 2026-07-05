#pragma once
#include <vector>
#include <string>
#include "Entities/GameObject.h"
#include "Core/EnemyPathManager.h"
class Player;

class LevelManager : public IEnemyObserver {
private:
    std::vector<GameObject*> levelEntities;
    std::vector<std::string> levelGrid;
    std::vector<Enemy*> pendingRemoval;
    EnemyPathManager enemyPathManager;

    float levelWidth;
    float levelHeight;

    void ProcessPendingRemovals();
public:
    LevelManager();
    ~LevelManager();

    void LoadLevel(const std::string& filepath, Player* player);
    void DrawLevel();
    void UpdateLevel(float deltaTime);
    void ClearLevel();
    void AddEntity(GameObject* entity);
    bool IsValidSpawnLocation(Vector2 position) const;
    
    float GetLevelWidth() const { return levelWidth; }
    float GetLevelHeight() const { return levelHeight; }

    EnemyPathManager* GetPathManager() { return &enemyPathManager; }

    // Helper function for levelGrid usage
    char GetTile(int x, int y) const;
    bool IsWalkableTile(int x, int y) const;
    Vector2 WorldToTile(Vector2 worldPos) const;
    Vector2 TileToWorld(int tileX, int tileY) const;

    // Override functions of Enemy Observer
    void OnEnemyPathFind(Enemy* enemy) override;
    void OnEnemyPathFindEnded(Enemy* enemy) override;
    void OnEnemyDied(Enemy* enemy) override;
    
    const std::vector<GameObject*>& GetEntities() const { return levelEntities; }
};
