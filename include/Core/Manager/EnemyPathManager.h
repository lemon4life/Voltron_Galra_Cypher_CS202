#pragma once
#include "raylib.h"

#include <vector>

class LevelManager;
class PathfindingEnemy;

class EnemyPathManager {
private:
    std::vector<PathfindingEnemy*> enemies;
    int nextEnemyIndex = 0;
public:
    void AddEnemy(PathfindingEnemy* enemy);

    void RemoveEnemy(PathfindingEnemy* enemy);
    void Clear();

    Vector2 GetNextMoveTarget(
        LevelManager* levelManager,
        PathfindingEnemy* enemy,
        Vector2 fallbackTarget
    );

    Vector2 GetLocalAvoidanceDirection(
        LevelManager* levelManager,
        PathfindingEnemy* enemy,
        Vector2 desiredDirection
    );

    void Update(LevelManager* levelManager, float deltaTime);
};
