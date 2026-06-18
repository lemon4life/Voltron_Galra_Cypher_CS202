#pragma once
#include "IAttackStrategy.h"

class MeleeAttackStrategy : public IAttackStrategy {
private:
    Rectangle hitbox;
    float attackTimer;
    bool isActive;

public:
    MeleeAttackStrategy();
    
    void Attack(Vector2 playerPos, bool facingLeft) override;
    void Update(float deltaTime) override;
    void Draw() override;
};
