#pragma once
#include "Entities/GameObject.h"
#include "AI/EnemyState.h"
#include "AI/EnemyCollision.h"
#include "Core/LevelAccess.h"
#include "Combat/WeaponKinematics.h"
#include "Entities/Components/StatusComponent.h"
#include "raylib.h"

#include <deque>
#include <memory>
#include <utility>
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

struct EnemyCollisionProfile {
    Vector2 navigationSize = { 16.0f, 8.0f };
    Vector2 navigationCenterOffset = { 0.0f, 8.0f };
};

struct EnemyPathDebugPoint {
    Vector2 position;
    bool hasLineOfSight;
};

class Enemy : public GameObject {
protected:
    int health;
    int maxHealth;
    float speed;
    int damage;
    float attackCooldown;
    float baseAttackCooldown;
    float dazeDuration;
    float knockbackResistance;
    Vector2 size;
    Vector2 renderFootOffset = { 0.0f, 0.0f };
    EnemyCollisionProfile collisionProfile;
    Vector2 knockbackVelocity;
    Vector2 currentVelocity = {0.0f, 0.0f};

    EnemyType enemyType;

    EnemySprites sprites;
    bool facingLeft = false;
    bool movedThisFrame = false;
    float runFrameTime = 0.0f;
    int currentRunFrame = 0;
    
    float weaponAngle = 0.0f;
    
    bool playingEffect = false;
    float effectTimer = 0.0f;
    int currentEffectFrame = 0;

    WeaponKinematics kinematics;
    StatusComponent statusComponent;

    TeamManager* targetTeam;
    IEnemyState* currentState;

    std::unique_ptr<IEnemyState> idleState;
    std::unique_ptr<IEnemyState> chaseState;
    std::unique_ptr<EnemyDazeState> dazeState;

    bool deathNotified = false;
    IEntityRemovalAccess& removalAccess;
    IEnemyPathAccess& pathAccess;

private:
    bool spawnSequenceActive = false;
    float spawnSequenceElapsed = 0.0f;
    Texture2D spawnEffectTexture = { 0 };
    bool usePathFinding = false;
    std::deque<Vector2> targetPositions;
    EnemyPathStatus pathStatus = EnemyPathStatus::Pending;
    std::vector<EnemyPathDebugPoint> pathDebugPoints;
    Vector2 selectedPathGoal = { 0.0f, 0.0f };
    bool hasSelectedPathGoal = false;

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
    void DrawPathDebug() const;

    IEnemyState* GetCurrentState() { return currentState; }
    IEnemyState* GetIdleState() { return idleState.get(); }
    IEnemyState* GetChaseState() { return chaseState.get(); }
    EnemyDazeState* GetDazeState() { return dazeState.get(); }
    void ChangeState(IEnemyState* newState);

    virtual void TakeDamage(int amount);
    void BeginSpawnSequence();
    bool UpdateSpawnSequence(float deltaTime);
    bool IsEnabled() const { return !spawnSequenceActive; }
    bool IsFacingLeft() const { return facingLeft; }
    bool ShouldDrawDuringSpawn() const;
    void DrawSpawnEffect() const;
    void ApplyKnockback(Vector2 dir, float force);
    void ApplyCollisionPush(Vector2 dir, float distance);
    void UpdateKnockback(float deltaTime);
    EnemyMoveResult UpdateMovement(Vector2 desiredVelocity, float deltaTime, EnemyWallResponse response = EnemyWallResponse::Slide);
    bool IsDead() const { return health <= 0; }
    bool CheckCollision(const std::vector<GameObject*>& entities) const;

    Rectangle GetBoundingBox() const override;
    Rectangle GetCollisionBox() const override;
    Rectangle GetNavigationFootprintAt(Vector2 entityPosition) const;
    Rectangle GetContactAttackBoxAt(Vector2 entityPosition) const;

    void SetEnemySprites(EnemySprites s) { sprites = s; }
    WeaponKinematics& GetKinematics() { return kinematics; }
    EnemyType GetEnemyType() const { return enemyType; }
    EnemySprites GetSprites() const { return sprites; }

    int GetHealth() const { return health; }
    void SetHealth(int h);
    void SetMaxHealth(int h);
    int GetMaxHealth() const { return maxHealth; }
    float GetSpeed() const { 
        return statusComponent.HasEffect(EffectType::SLOW) ? speed * 0.5f : speed; 
    }
    void SetSpeed(float s) { speed = s; }
    int GetDamage() const { return damage; }
    void SetDamage(int value) { damage = value; }
    float GetAttackCooldown() const { return attackCooldown; }
    void SetAttackCooldown(float cd) { attackCooldown = cd; }
    float GetBaseAttackCooldown() const { return baseAttackCooldown; }
    void SetBaseAttackCooldown(float cd) { baseAttackCooldown = cd; }
    void ResetAttackCooldown();
    float GetDazeDuration() const { return dazeDuration; }
    void SetDazeDuration(float duration) { dazeDuration = duration; }
    bool IsKnockedBack() const { return knockbackVelocity.x != 0.0f || knockbackVelocity.y != 0.0f; }
    Vector2 GetKnockbackVelocity() const { return knockbackVelocity; }

    // Scalar scaling for Roguelike progression
    void ApplyStatMultiplier(float multiplier);

    void SetKnockbackResistance(float resistance);
    float GetKnockbackMultiplier() const {
        return 1.0f - knockbackResistance;
    }
    float GetDazeTimeRemaining() const {
        return dazeState ? dazeState->GetRemainingTime() : 0.0f;
    }
    Vector2 GetSize() const { return size; }
    void SetSize(Vector2 value) { size = value; }
    Vector2 GetRenderFootPosition() const {
        return {
            position.x + renderFootOffset.x,
            position.y + renderFootOffset.y
        };
    }
    void SetRenderFootOffset(Vector2 offset) {
        renderFootOffset = offset;
    }

    StatusComponent& GetStatusComponent() { return statusComponent; }
    void SetCurrentVelocity(Vector2 v) { currentVelocity = v; }
    bool IsMovingForAnimation() const { return movedThisFrame; }
    void UpdateMovementAnimationFlag(Vector2 updateStartPosition) {
        constexpr float MOVEMENT_EPSILON_SQUARED = 0.0001f;
        float deltaX = position.x - updateStartPosition.x;
        float deltaY = position.y - updateStartPosition.y;
        movedThisFrame = deltaX * deltaX + deltaY * deltaY >
            MOVEMENT_EPSILON_SQUARED;
    }

    EnemyCollisionProfile GetCollisionProfile() const {
        return collisionProfile;
    }
    void SetCollisionProfile(EnemyCollisionProfile profile) {
        collisionProfile = profile;
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
    bool HasTargetPosition() const { return !targetPositions.empty(); }
    void SetPathStatus(EnemyPathStatus status) { pathStatus = status; }
    EnemyPathStatus GetPathStatus() const { return pathStatus; }
    void SetPathDebugPoints(std::vector<EnemyPathDebugPoint> points) {
        pathDebugPoints = std::move(points);
    }
    void SetSelectedPathGoal(Vector2 goal) {
        selectedPathGoal = goal;
        hasSelectedPathGoal = true;
    }
    void ClearSelectedPathGoal() { hasSelectedPathGoal = false; }

    TeamManager* GetTargetTeam() const { return targetTeam; }
};
