#pragma once
#include "Entities/GameObject.h"
#include "AI/EnemyState.h"
#include <vector>

class Player;

enum class EnemyType {
    GRUNT,
    BOSS
};

class Enemy : public GameObject {
private:
    EnemyType type;
    int health;
    int maxHealth;
    float speed;
    int damage;

    Player* target;
    IEnemyState* currentState;

    EnemyIdleState idleState;
    EnemyChaseState chaseState;
    BossRangedAttackState bossRangedState;

    float attackCooldown;
    
    // Boss specific
    float bossSkillCooldown;
    int burstCount;
    float burstTimer;

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

    EnemyType GetType() const { return type; }
    void SetType(EnemyType t) { type = t; }

    int GetHealth() const { return health; }
    int GetMaxHealth() const { return maxHealth; }
    void SetMaxHealth(int h) { maxHealth = h; health = h; }
    float GetSpeed() const { return speed; }
    void SetSpeed(float s) { speed = s; }
    int GetDamage() const { return damage; }
    Player* GetTarget() const { return target; }
    
    float GetAttackCooldown() const { return attackCooldown; }
    void SetAttackCooldown(float cd) { attackCooldown = cd; }
    
    float GetBossSkillCooldown() const { return bossSkillCooldown; }
    void SetBossSkillCooldown(float cd) { bossSkillCooldown = cd; }
    int GetBurstCount() const { return burstCount; }
    void SetBurstCount(int count) { burstCount = count; }
    float GetBurstTimer() const { return burstTimer; }
    void SetBurstTimer(float timer) { burstTimer = timer; }
    
    EnemyIdleState* GetIdleState() { return &idleState; }
    EnemyChaseState* GetChaseState() { return &chaseState; }
    BossRangedAttackState* GetBossRangedAttackState() { return &bossRangedState; }
};
