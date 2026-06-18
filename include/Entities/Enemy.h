#pragma once
#include "Entities/GameObject.h"
#include "AI/EnemyState.h"
#include <vector>

class Player;

class Enemy : public GameObject {
private:
    int health;
    int maxHealth;
    float speed;
    int damage;

    Player* target;
    IEnemyState* currentState;

    EnemyIdleState idleState;
    EnemyChaseState chaseState;

    float attackCooldown;

public:
    Enemy(Vector2 pos, Player* t);
    ~Enemy() override;

    void Update(float deltaTime) override;
    void Draw() override;

    void ChangeState(IEnemyState* newState);

    void TakeDamage(int amount);
    bool IsDead() const { return health <= 0; }
    bool CheckCollision(const std::vector<GameObject*>& entities) const;

    Rectangle GetBoundingBox() const override;

    int GetHealth() const { return health; }
    float GetSpeed() const { return speed; }
    int GetDamage() const { return damage; }
    Player* GetTarget() const { return target; }
    
    float GetAttackCooldown() const { return attackCooldown; }
    void SetAttackCooldown(float cd) { attackCooldown = cd; }
    
    EnemyIdleState* GetIdleState() { return &idleState; }
    EnemyChaseState* GetChaseState() { return &chaseState; }
};
