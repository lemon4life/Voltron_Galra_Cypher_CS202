#pragma once
#include "IAttackStrategy.h"

class RangedAttackStrategy : public IAttackStrategy {
private:
    Texture2D weaponTex;

public:
    RangedAttackStrategy(Texture2D tex);
    void Attack(Vector2 playerPos) override;
    void Update(float deltaTime) override;
    void Draw(Vector2 playerPos, bool facingLeft) override;
};
