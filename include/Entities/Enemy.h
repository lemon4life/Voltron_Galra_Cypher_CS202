#pragma once
#include "Entities/GameObject.h"
#include "AI/EnemyState.h"
#include "AI/EnemyCollision.h"
#include "Core/LevelAccess.h"
#include "Combat/WeaponKinematics.h"
#include "raylib.h"

#include <deque>
#include <memory>
#include <vector>


struct EnemySprites {
    Texture2D idle;
    Texture2D run;
    Texture2D down;
    Texture2D weapon;
    Texture2D effect;
    Texture2D projectile;
};

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
    float dazeDuration;
    Vector2 size;
    Vector2 knockbackVelocity;
    Vector2 currentVelocity = {0.0f, 0.0f};

    EnemyType enemyType;

    EnemySprites sprites;
    bool facingLeft = false;
    float runFrameTime = 0.0f;
    int currentRunFrame = 0;
    
    float weaponAngle = 0.0f;
    
    bool playingEffect = false;
    float effectTimer = 0.0f;
    int currentEffectFrame = 0;

    WeaponKinematics kinematics;

    TeamManager* targetTeam;
    IEnemyState* currentState;

    std::unique_ptr<IEnemyState> idleState;
    std::unique_ptr<IEnemyState> chaseState;
    std::unique_ptr<EnemyDazeState> dazeState;

    bool deathNotified = false;
    IEntityRemovalAccess& removalAccess;
    IEnemyPathAccess& pathAccess;

private:
    bool usePathFinding = false;
    std::deque<Vector2> targetPositions;

public:
    Enemy(
        Vector2 pos,
        TeamManager* t,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess
    );
    Enemy(
        Vector2 pos,
        TeamManager* t,
        int maxHealth,
        float speed,
        int damage,
        float attackCooldown,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess
    );
    virtual ~Enemy();

    virtual void Update(float deltaTime) override = 0;
    void Draw() override {};

    IEnemyState* GetCurrentState() { return currentState; }
    IEnemyState* GetIdleState() { return idleState.get(); }
    IEnemyState* GetChaseState() { return chaseState.get(); }
    EnemyDazeState* GetDazeState() { return dazeState.get(); }
    void ChangeState(IEnemyState* newState);

    virtual void TakeDamage(int amount);
    void ApplyKnockback(Vector2 dir, float force);
    void UpdateKnockback(float deltaTime);
    EnemyMoveResult UpdateMovement(Vector2 desiredVelocity, float deltaTime, EnemyWallResponse response = EnemyWallResponse::Slide);
    bool IsDead() const { return health <= 0; }
    bool CheckCollision(const std::vector<GameObject*>& entities) const;

    Rectangle GetBoundingBox() const override;
    Rectangle GetCollisionBox() const override;

    void SetEnemySprites(EnemySprites s) { sprites = s; }
    WeaponKinematics& GetKinematics() { return kinematics; }
    EnemyType GetEnemyType() const { return enemyType; }
    EnemySprites GetSprites() const { return sprites; }

    int GetHealth() const { return health; }
    void SetMaxHealth(int h) { maxHealth = h; health = h; }
    int GetMaxHealth() const { return maxHealth; }
    float GetSpeed() const { return speed; }
    void SetSpeed(float s) { speed = s; }
    int GetDamage() const { return damage; }
    float GetAttackCooldown() const { return attackCooldown; }
    void SetAttackCooldown(float cd) { attackCooldown = cd; }
    void ResetAttackCooldown();
    float GetDazeDuration() const { return dazeDuration; }
    void SetDazeDuration(float duration) { dazeDuration = duration; }
    float GetDazeTimeRemaining() const {
        return dazeState ? dazeState->GetRemainingTime() : 0.0f;
    }

    IEnemyPathAccess& GetPathAccess() const { return pathAccess; }
    void StartPathFinding();
    void EndPathFinding();
    bool IsPathFinding() const { return usePathFinding; }

    void ClearTargetPosition() { targetPositions.clear(); }
    void PopTarget() {
        if (!targetPositions.empty()) targetPositions.pop_front();
    }
    void AddTargetPosition(Vector2 targetPosition) {
        targetPositions.push_back(targetPosition);
    }
    Vector2 FirstTargetPosition() const {
        return targetPositions.empty()
            ? Vector2{ -1.0f, -1.0f }
            : targetPositions.front();
    }
    const std::deque<Vector2>& GetTargetPositions() const {
        return targetPositions;
    }
    bool HasTargetPosition() const { return !targetPositions.empty(); }

    TeamManager* GetTargetTeam() const { return targetTeam; }
};
