#pragma once
#include "Entities/GameObject.h"
#include "AI/EnemyState.h"
#include "Core/LevelAccess.h"
#include "raylib.h"

#include <memory>
#include <vector>

class TeamManager;

enum class EnemyType {
    GRUNT,
    BOSS,
    Chaser,
    RANGE,
    DIVER
};

class Enemy : public GameObject {
protected:
    int health;
    int maxHealth;
    float speed;
    int damage;
    float attackCooldown;
    const float baseAttackCooldown;
    Vector2 size;
    Vector2 knockbackVelocity;

    EnemyType enemyType;

    TeamManager* targetTeam;
    IEnemyState* currentState;

    std::unique_ptr<IEnemyState> idleState;
    std::unique_ptr<IEnemyState> chaseState;

    bool deathNotified = false;
    IEntityRemovalAccess* removalAccess;

public:
    Enemy(Vector2 pos, TeamManager* t, IEntityRemovalAccess* removalAccess);
    Enemy(
        Vector2 pos,
        TeamManager* t,
        int maxHealth,
        float speed,
        int damage,
        float attackCooldown,
        IEntityRemovalAccess* removalAccess
    );
    virtual ~Enemy();

    virtual void Update(float deltaTime) override = 0;
    void Draw() override {};

    IEnemyState* GetCurrentState() { return currentState; }
    IEnemyState* GetIdleState() { return idleState.get(); }
    IEnemyState* GetChaseState() { return chaseState.get(); }
    void ChangeState(IEnemyState* newState);

    virtual void TakeDamage(int amount);
    void ApplyKnockback(Vector2 dir, float force);
    void UpdateKnockback(float deltaTime);
    bool IsDead() const { return health <= 0; }
    bool CheckCollision(const std::vector<GameObject*>& entities) const;

    Rectangle GetBoundingBox() const override;

    EnemyType GetEnemyType() const { return enemyType; }

    int GetHealth() const { return health; }
    void SetMaxHealth(int h) { maxHealth = h; health = h; }
    int GetMaxHealth() const { return maxHealth; }
    float GetSpeed() const { return speed; }
    void SetSpeed(float s) { speed = s; }
    int GetDamage() const { return damage; }
    float GetAttackCooldown() const { return attackCooldown; }
    void SetAttackCooldown(float cd) { attackCooldown = cd; }
    void ResetAttackCooldown();

    TeamManager* GetTargetTeam() const { return targetTeam; }
};
