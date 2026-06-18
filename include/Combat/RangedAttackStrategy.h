#pragma once
#include "IAttackStrategy.h"

class RangedAttackStrategy : public IAttackStrategy {
public:
    void Attack(Vector2 playerPos, bool facingLeft) override;
    void Update(float deltaTime) override;
    void Draw() override;
};
