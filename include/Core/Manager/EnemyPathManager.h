#pragma once
#include "raylib.h"

#include <vector>

class LevelManager;
class Enemy;

class EnemyPathManager {
private:
    std::vector<Enemy*> enemies;
    int nextEnemyIndex = 0;
public:
    void AddEnemy(Enemy& enemy);

    void RemoveEnemy(Enemy& enemy);
    void Clear();

    Vector2 GetNextMoveTarget(
        LevelManager& levelManager,
        Enemy& enemy,
        Vector2 fallbackTarget
    );

    Vector2 GetLocalAvoidanceDirection(
        LevelManager& levelManager,
        Enemy& enemy,
        Vector2 desiredDirection
    );

    void Update(LevelManager& levelManager, float deltaTime);
};
