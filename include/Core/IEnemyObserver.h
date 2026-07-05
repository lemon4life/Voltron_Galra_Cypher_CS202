#pragma once

class Enemy;

// Observer interface for enemy events
class IEnemyObserver {
public:
    virtual ~IEnemyObserver() = default;

    virtual void OnEnemyPathFind(Enemy* enemy) {};
    virtual void OnEnemyPathFindEnded(Enemy* enemy) {};
    virtual void OnEnemyDied(Enemy* enemy) {};
};