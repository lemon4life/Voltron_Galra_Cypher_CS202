#pragma once
#include "Entities/GameObject.h"
#include "AI/EnemyState.h"
#include "Core/IEnemyObserver.h"
#include "raylib.h"

#include <memory>
#include <vector>

class Player;

enum class EnemyType {
    Chaser
};

class Enemy : public GameObject {
protected:
    int health  = 100;
    int maxHealth = 100;
    float speed = 100.f;
    int damage = 15;
    float attackCooldown = 0.1f;

    Vector2 size;

    EnemyType enemyType;

    Player* target;
    IEnemyState* currentState;

    std::unique_ptr<IEnemyState> idleState;
    std::unique_ptr<IEnemyState> chaseState;

    bool deathNotified = false;
    std::vector<IEnemyObserver*> observers;

protected:
    void NotifyEnemyDied();

public:
    Enemy(Vector2 pos, Player* t);
    Enemy(Vector2 pos, Player* t, int maxHealth, float speed, int damage, float attackCooldown);
    virtual ~Enemy();

    IEnemyState* GetCurrentState() { return currentState; }
    void ToIdleState();
    void ToChaseState();

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
    float GetAttackCooldown() const { return attackCooldown; }
    void SetAttackCooldown(float cd) { attackCooldown = cd; }

    Player* GetTarget() const { return target; }
};
