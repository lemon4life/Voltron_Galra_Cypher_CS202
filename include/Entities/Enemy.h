#pragma once
#include "Entities/GameObject.h"
#include "AI/EnemyState.h"
#include "Core/IEnemyObserver.h"
#include <vector>

class Player;

class Enemy : public GameObject {
protected:
    int health  = 100;
    int maxHealth = 100;
    float speed = 100.f;
    int damage = 15;
    float attackCooldown = 0.1f;

    Player* target;
    IEnemyState* currentState;

    EnemyIdleState idleState;
    EnemyChaseState chaseState;

    bool deathNotified = false;
    std::vector<IEnemyObserver*> observers;

protected:
    void NotifyEnemyPathFind();
    void NotifyEnemyPathFindEnded();
    void NotifyEnemyDied();

public:
    Enemy(Vector2 pos, Player* t);
    Enemy(Vector2 pos, Player* t, int maxHealth, float speed, int damage, float attackCooldown);
    ~Enemy() override;

    void Update(float deltaTime) override;
    void Draw() override;

    IEnemyState* GetCurrentState() { return currentState; }
    void ChangeState(IEnemyState* newState);

    void AddObserver(IEnemyObserver* observer);
    void RemoveObserver(IEnemyObserver* observer);

    virtual void TakeDamage(int amount);
    bool IsDead() const { return health <= 0; }
    bool CheckCollision(const std::vector<GameObject*>& entities) const;

    Rectangle GetBoundingBox() const override;

    int GetHealth() const { return health; }
    int GetMaxHealth() const { return maxHealth; }
    float GetSpeed() const { return speed; }
    int GetDamage() const { return damage; }
    Player* GetTarget() const { return target; }
    virtual void SetTargetPosition(Vector2 target) {}
    virtual Vector2 GetTargetPosition() const { return position; }
    virtual bool HasTargetPosition() const { return false; }
    
    float GetAttackCooldown() const { return attackCooldown; }
    void SetAttackCooldown(float cd) { attackCooldown = cd; }
    
    virtual IEnemyState* GetIdleState() { return &idleState; }
    virtual IEnemyState* GetChaseState() { return &chaseState; }
};
