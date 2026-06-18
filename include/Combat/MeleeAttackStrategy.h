#pragma once
#include "IAttackStrategy.h"
#include <vector>

class Enemy;

class MeleeAttackStrategy : public IAttackStrategy {
private:
    Rectangle hitbox;
    float attackTimer;
    bool isActive;
    std::vector<Enemy*> enemiesHit;

public:
    MeleeAttackStrategy();
    
    void Attack(Vector2 playerPos, bool facingLeft) override;
    void Update(float deltaTime) override;
    void Draw() override;
};
