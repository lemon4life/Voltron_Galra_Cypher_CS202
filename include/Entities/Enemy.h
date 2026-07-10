#pragma once
#include "Entities/GameObject.h"
#include "AI/EnemyState.h"
#include "Core/IEnemyObserver.h"
#include "raylib.h"

#include <memory>
#include <vector>

class TeamManager;

enum class EnemyType {
    GRUNT,
    BOSS,
    Chaser
};

class Enemy : public GameObject {
protected:
    int health;
    int maxHealth;
    float speed;
    int damage;
    float attackCooldown;
    Vector2 size;

    EnemyType enemyType;

    TeamManager* targetTeam;
    IEnemyState* currentState;

    std::unique_ptr<IEnemyState> idleState;
    std::unique_ptr<IEnemyState> chaseState;

    bool deathNotified = false;
    std::vector<IEnemyObserver*> observers;

    void NotifyEnemyDied();

public:
    Enemy(Vector2 pos, TeamManager* t);
    Enemy(Vector2 pos, TeamManager* t, int maxHealth, float speed, int damage, float attackCooldown);
    virtual ~Enemy();

    virtual void Update(float deltaTime) override = 0;
    void Draw() override {};

    IEnemyState* GetCurrentState() { return currentState; }
    IEnemyState* GetIdleState() { return idleState.get(); }
    IEnemyState* GetChaseState() { return chaseState.get(); }
    void ChangeState(IEnemyState* newState);

    void AddObserver(IEnemyObserver* observer);
    void RemoveObserver(IEnemyObserver* observer);

    virtual void TakeDamage(int amount);
    bool IsDead() const { return health <= 0; }
    bool CheckCollision(const std::vector<GameObject*>& entities) const;

    Rectangle GetBoundingBox() const override;

    EnemyType GetType() const { return enemyType; }
    void SetType(EnemyType t) { enemyType = t; }
    EnemyType GetEnemyType() const { return enemyType; }
    void SetEnemyType(EnemyType t) { enemyType = t; }

    int GetHealth() const { return health; }
    void SetMaxHealth(int h) { maxHealth = h; health = h; }
    int GetMaxHealth() const { return maxHealth; }
    float GetSpeed() const { return speed; }
    void SetSpeed(float s) { speed = s; }
    int GetDamage() const { return damage; }
    float GetAttackCooldown() const { return attackCooldown; }
    void SetAttackCooldown(float cd) { attackCooldown = cd; }

    TeamManager* GetTargetTeam() const { return targetTeam; }
};
