#pragma once
#include "Core/IEnemyObserver.h"
#include "raylib.h"
#include <algorithm>
#include <vector>

const float TARGET_LOOP_ALL_INTERVAL = 0.2;

class LevelManager;

class EnemyPathManager {
private:
    std::vector<Enemy*> enemies;
    int nextEnemyIndex = 0;
public:
    void AddEnemy(Enemy* enemy);

    void RemoveEnemy(Enemy* enemy);

    Vector2 GetNextMoveTarget(LevelManager* levelManager, Enemy* enemy, Vector2 fallbackTarget);

    Vector2 GetLocalAvoidanceDirection(LevelManager* levelManager, Enemy* enemy, Vector2 desiredDirection);

    void Update(LevelManager* levelManager, float deltaTime);
};
