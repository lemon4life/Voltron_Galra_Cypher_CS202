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
    DIVER,
    DRONE,
    DEMON_THA
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
    bool localEnemyAvoidanceEnabled = true;
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
    /// Creates a Enemy instance from the supplied configuration.
    Enemy(
        Vector2 pos,
        TeamManager* t,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess
    );
    /// Creates a Enemy instance from the supplied configuration.
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
    /// Releases resources owned by this Enemy instance.
    virtual ~Enemy();

    /// Advances this component's state for the current frame.
    virtual void Update(float deltaTime) override = 0;
    /// Renders this component using its current state and visual resources.
    void Draw() override {};
    /// Renders path debug.
    void DrawPathDebug() const;

    /// Returns the current current state.
    IEnemyState* GetCurrentState() { return currentState; }
    /// Returns the current idle state.
    IEnemyState* GetIdleState() { return idleState.get(); }
    /// Returns the current chase state.
    IEnemyState* GetChaseState() { return chaseState.get(); }
    /// Returns the current daze state.
    EnemyDazeState* GetDazeState() { return dazeState.get(); }
    /// Leaves the current state, switches ownership, and enters the replacement state.
    void ChangeState(IEnemyState* newState);

    /// Applies incoming damage after this object handles defenses and state-specific rules.
    virtual void TakeDamage(int amount);
    /// Begins spawn sequence.
    void BeginSpawnSequence();
    /// Updates spawn sequence.
    bool UpdateSpawnSequence(float deltaTime);
    /// Reports whether the enabled condition is satisfied.
    bool IsEnabled() const { return !spawnSequenceActive; }
    /// Reports whether the spawn sequence active condition is satisfied.
    bool IsSpawnSequenceActive() const { return spawnSequenceActive; }
    /// Reports whether the facing left condition is satisfied.
    bool IsFacingLeft() const { return facingLeft; }
    /// Updates the stored facing left.
    void SetFacingLeft(bool b) { facingLeft = b; }
    /// Reports whether this component should perform draw during spawn.
    bool ShouldDrawDuringSpawn() const;
    /// Renders spawn effect.
    void DrawSpawnEffect() const;
    /// Applies knockback.
    void ApplyKnockback(Vector2 dir, float force);
    /// Applies collision push.
    void ApplyCollisionPush(Vector2 dir, float distance);
    /// Updates knockback.
    void UpdateKnockback(float deltaTime);
    /// Updates movement.
    EnemyMoveResult UpdateMovement(Vector2 desiredVelocity, float deltaTime, EnemyWallResponse response = EnemyWallResponse::Slide);
    /// Reports whether the dead condition is satisfied.
    bool IsDead() const { return health <= 0; }

    /// Returns the current bounding box.
    Rectangle GetBoundingBox() const override;
    /// Returns the current collision box.
    Rectangle GetCollisionBox() const override;
    /// Returns the current navigation footprint at.
    Rectangle GetNavigationFootprintAt(Vector2 entityPosition) const;
    /// Returns the current contact attack box at.
    Rectangle GetContactAttackBoxAt(Vector2 entityPosition) const;

    /// Updates the stored enemy sprites.
    void SetEnemySprites(EnemySprites s) { sprites = s; }
    /// Returns the current kinematics.
    WeaponKinematics& GetKinematics() { return kinematics; }
    /// Returns the current enemy type.
    EnemyType GetEnemyType() const { return enemyType; }
    /// Returns the current sprites.
    EnemySprites GetSprites() const { return sprites; }

    /// Returns the current health.
    int GetHealth() const { return health; }
    /// Updates the stored health.
    void SetHealth(int h);
    /// Updates the stored max health.
    void SetMaxHealth(int h);
    /// Returns the current max health.
    int GetMaxHealth() const { return maxHealth; }
    /// Returns the current speed.
    float GetSpeed() const { 
        return statusComponent.HasEffect(EffectType::SLOW) ? speed * 0.5f : speed; 
    }
    /// Updates the stored speed.
    void SetSpeed(float value);
    /// Returns the current damage.
    int GetDamage() const { return damage; }
    /// Updates the stored damage.
    void SetDamage(int value);
    /// Returns the current attack cooldown.
    float GetAttackCooldown() const { return attackCooldown; }
    /// Updates the stored attack cooldown.
    void SetAttackCooldown(float value);
    /// Returns the current base attack cooldown.
    float GetBaseAttackCooldown() const { return baseAttackCooldown; }
    /// Updates the stored base attack cooldown.
    void SetBaseAttackCooldown(float value);
    /// Resets attack cooldown.
    void ResetAttackCooldown();
    /// Returns the current daze duration.
    float GetDazeDuration() const { return dazeDuration; }
    /// Updates the stored daze duration.
    void SetDazeDuration(float duration);
    /// Reports whether the knocked back condition is satisfied.
    bool IsKnockedBack() const { return knockbackVelocity.x != 0.0f || knockbackVelocity.y != 0.0f; }
    /// Returns the current knockback velocity.
    Vector2 GetKnockbackVelocity() const { return knockbackVelocity; }

    // Scalar scaling for Roguelike progression
    /// Applies stat multipliers.
    void ApplyStatMultipliers(
        float healthMultiplier,
        float damageMultiplier,
        float speedMultiplier
    );

    /// Updates the stored knockback resistance.
    void SetKnockbackResistance(float resistance);
    /// Returns the current knockback multiplier.
    float GetKnockbackMultiplier() const {
        return 1.0f - knockbackResistance;
    }
    /// Returns the current daze time remaining.
    float GetDazeTimeRemaining() const {
        return dazeState ? dazeState->GetRemainingTime() : 0.0f;
    }
    /// Returns the current size.
    Vector2 GetSize() const { return size; }
    /// Updates the stored size.
    void SetSize(Vector2 value);
    /// Returns the current render foot position.
    Vector2 GetRenderFootPosition() const {
        return {
            position.x + renderFootOffset.x,
            position.y + renderFootOffset.y
        };
    }
    /// Updates the stored render foot offset.
    void SetRenderFootOffset(Vector2 offset) {
        renderFootOffset = offset;
    }

    /// Returns the current status component.
    StatusComponent& GetStatusComponent() { return statusComponent; }
    /// Updates the stored current velocity.
    void SetCurrentVelocity(Vector2 v) { currentVelocity = v; }
    /// Returns the current current velocity.
    Vector2 GetCurrentVelocity() const { return currentVelocity; }
    /// Reports whether the moving for animation condition is satisfied.
    bool IsMovingForAnimation() const { return movedThisFrame; }
    /// Updates movement animation flag.
    void UpdateMovementAnimationFlag(Vector2 updateStartPosition) {
        constexpr float MOVEMENT_EPSILON_SQUARED = 0.0001f;
        float deltaX = position.x - updateStartPosition.x;
        float deltaY = position.y - updateStartPosition.y;
        movedThisFrame = deltaX * deltaX + deltaY * deltaY >
            MOVEMENT_EPSILON_SQUARED;
    }

    /// Returns the current collision profile.
    EnemyCollisionProfile GetCollisionProfile() const {
        return collisionProfile;
    }
    /// Updates the stored collision profile.
    void SetCollisionProfile(EnemyCollisionProfile profile);

    /// Returns the current path access.
    IEnemyPathAccess& GetPathAccess() const { return pathAccess; }
    /// Starts path finding.
    void StartPathFinding();
    /// Starts path finding to.
    void StartPathFindingTo(Vector2 worldGoal);
    /// Finishes path finding.
    void EndPathFinding();
    /// Reports whether the path finding condition is satisfied.
    bool IsPathFinding() const { return usePathFinding; }
    /// Reports whether the local enemy avoidance enabled condition is satisfied.
    bool IsLocalEnemyAvoidanceEnabled() const {
        return localEnemyAvoidanceEnabled;
    }
    /// Updates the stored local enemy avoidance enabled.
    void SetLocalEnemyAvoidanceEnabled(bool enabled) {
        localEnemyAvoidanceEnabled = enabled;
    }

    /// Clears target position.
    void ClearTargetPosition() { targetPositions.clear(); }
    /// Implements the pop target behavior for this component.
    void PopTarget() {
        if (!targetPositions.empty()) targetPositions.pop_front();
    }
    /// Adds target position.
    void AddTargetPosition(Vector2 targetPosition) {
        targetPositions.push_back(targetPosition);
    }
    /// Implements the first target position behavior for this component.
    Vector2 FirstTargetPosition() const {
        return targetPositions.empty()
            ? Vector2{ -1.0f, -1.0f }
            : targetPositions.front();
    }
    /// Reports whether this component has target position.
    bool HasTargetPosition() const { return !targetPositions.empty(); }
    /// Updates the stored path status.
    void SetPathStatus(EnemyPathStatus status) { pathStatus = status; }
    /// Returns the current path status.
    EnemyPathStatus GetPathStatus() const { return pathStatus; }
    /// Updates the stored path debug points.
    void SetPathDebugPoints(std::vector<EnemyPathDebugPoint> points) {
        pathDebugPoints = std::move(points);
    }
    /// Updates the stored selected path goal.
    void SetSelectedPathGoal(Vector2 goal) {
        selectedPathGoal = goal;
        hasSelectedPathGoal = true;
    }
    /// Clears selected path goal.
    void ClearSelectedPathGoal() { hasSelectedPathGoal = false; }

    /// Returns the current target team.
    TeamManager* GetTargetTeam() const { return targetTeam; }
};
