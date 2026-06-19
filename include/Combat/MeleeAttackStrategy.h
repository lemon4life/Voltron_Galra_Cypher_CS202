#pragma once
#include "IAttackStrategy.h"
#include <vector>

class GameObject;

class MeleeAttackStrategy : public IAttackStrategy {
private:
    bool isAttacking;
    float attackTimer;
    Rectangle hitbox;
    std::vector<GameObject*> enemiesHit;

    Texture2D weaponTex;

public:
    MeleeAttackStrategy(Texture2D tex);
    
    void Attack(Vector2 playerPos) override;
    void Update(float deltaTime) override;
    void Draw(Vector2 playerPos, bool facingLeft) override;
};
