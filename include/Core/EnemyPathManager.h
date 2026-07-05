#pragma once
#include "Core/IEnemyObserver.h"
#include <algorithm>
#include <vector>

class LevelManager;

class EnemyPathManager {
private:
    std::vector<Enemy*> enemies;
    int nextEnemyIndex = 0;
public:
    void AddEnemy(Enemy* enemy);

    void RemoveEnemy(Enemy* enemy);

    void Update(LevelManager* levelManager, float deltaTime);
};