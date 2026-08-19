#include "Entities/Enemy.h"
#include "AI/EnemyCollision.h"
#include "Core/Constants.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/AudioManager.h"
#include "raymath.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/ParticleManager.h"

#include <algorithm>

namespace {
    constexpr float DEBUG_PATH_THICKNESS = 2.0f;
    constexpr float DEBUG_WAYPOINT_RADIUS = 4.0f;
    constexpr float DEBUG_CURRENT_TARGET_RADIUS = 6.0f;
    constexpr float DEBUG_STATUS_OFFSET_Y = 22.0f;
    constexpr Color DEBUG_PATH_COLOR = { 255, 140, 0, 220 };
    constexpr Color DEBUG_WAYPOINT_COLOR = { 255, 230, 40, 255 };
    constexpr Color DEBUG_CURRENT_TARGET_COLOR = { 80, 255, 100, 255 };
    constexpr Color DEBUG_PENDING_COLOR = { 40, 220, 255, 255 };
    constexpr Color DEBUG_UNREACHABLE_COLOR = { 255, 60, 60, 255 };
    constexpr int SPAWN_EFFECT_FRAME_COUNT = 9;
    constexpr int SPAWN_BODY_VISIBLE_FRAME = 3;
    constexpr float SPAWN_EFFECT_FRAME_WIDTH = 64.0f;
    constexpr float SPAWN_EFFECT_FRAME_HEIGHT = 48.0f;
    constexpr float SPAWN_EFFECT_FRAME_DURATION = 0.1f;
    constexpr float SPAWN_POST_EFFECT_DELAY = 1.0f;
}

Enemy::Enemy(
    Vector2 pos,
    TeamManager* t,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess
)
    : GameObject(pos, GameObjectType::Enemy),
      health(100), maxHealth(100), speed(100.f), damage(15),
      attackCooldown(0.1f), baseAttackCooldown(0.1f), dazeDuration(2.0f),
      knockbackResistance(0.0f),
      size({32.0f, 32.0f}),
      knockbackVelocity{0.0f, 0.0f}, enemyType(EnemyType::GRUNT),
      targetTeam(t), currentState(nullptr), removalAccess(removalAccess),
      pathAccess(pathAccess)
{
    dazeState = std::make_unique<EnemyDazeState>();
}


Enemy::Enemy(
    Vector2 pos,
    TeamManager* t,
    int imaxHealth,
    float ispeed,
    int idamage,
    float iattackCooldown,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess
)
    : GameObject(pos, GameObjectType::Enemy),
      health(imaxHealth), maxHealth(imaxHealth), speed(ispeed),
      damage(idamage), attackCooldown(iattackCooldown),
      baseAttackCooldown(iattackCooldown), dazeDuration(2.0f),
      knockbackResistance(0.0f),
      size({32.0f, 32.0f}),
      knockbackVelocity{0.0f, 0.0f}, enemyType(EnemyType::GRUNT),
      targetTeam(t), currentState(nullptr), removalAccess(removalAccess),
      pathAccess(pathAccess)
{
    dazeState = std::make_unique<EnemyDazeState>();
}

Enemy::~Enemy() {
    EndPathFinding();
}

void Enemy::ChangeState(IEnemyState* newState) {
    if (!newState || currentState == newState) return;

    if (currentState) {
        currentState->Exit(this);
    }

    currentState = newState;
    currentState->Enter(this);
}

void Enemy::ResetAttackCooldown() {
    attackCooldown = baseAttackCooldown;
}

void Enemy::SetHealth(int value) {
    health = std::clamp(value, 0, maxHealth);
}

void Enemy::SetMaxHealth(int value) {
    maxHealth = std::max(1, value);
    health = std::min(health, maxHealth);
}

void Enemy::TakeDamage(int amount) {
    if (!IsEnabled() || health <= 0) return;

    int actualDamage = std::min(health, amount);
    health -= amount;
    if (health < 0) health = 0;
    
    if (actualDamage > 0) {
        GameManager::GetInstance().GetComboMeter().AddDamage(actualDamage);
        ParticleManager::GetInstance().SpawnDamageNumber(position, actualDamage);
    }
    
    AudioManager::GetInstance().PlaySoundEffect("hit");

    if (health <= 0 && !deathNotified) {
        deathNotified = true;
        removalAccess.QueueRemoval(this);
    }
}

void Enemy::BeginSpawnSequence() {
    EndPathFinding();
    spawnSequenceActive = true;
    spawnSequenceElapsed = 0.0f;
    spawnEffectTexture = AssetManager::GetInstance().GetTexture(
        "Enemy_Spawn"
    );
    currentVelocity = { 0.0f, 0.0f };
    knockbackVelocity = { 0.0f, 0.0f };
    movedThisFrame = false;
}

bool Enemy::UpdateSpawnSequence(float deltaTime) {
    if (!spawnSequenceActive) return false;

    currentVelocity = { 0.0f, 0.0f };
    knockbackVelocity = { 0.0f, 0.0f };
    movedThisFrame = false;
    spawnSequenceElapsed += std::max(0.0f, deltaTime);

    constexpr float EFFECT_DURATION =
        SPAWN_EFFECT_FRAME_COUNT * SPAWN_EFFECT_FRAME_DURATION;
    if (spawnSequenceElapsed >= EFFECT_DURATION + SPAWN_POST_EFFECT_DELAY) {
        spawnSequenceActive = false;
        return false;
    }

    return true;
}

bool Enemy::ShouldDrawDuringSpawn() const {
    if (!spawnSequenceActive) return true;

    return spawnSequenceElapsed >=
        SPAWN_BODY_VISIBLE_FRAME * SPAWN_EFFECT_FRAME_DURATION;
}

void Enemy::DrawSpawnEffect() const {
    constexpr float EFFECT_DURATION =
        SPAWN_EFFECT_FRAME_COUNT * SPAWN_EFFECT_FRAME_DURATION;
    if (!spawnSequenceActive ||
        spawnSequenceElapsed >= EFFECT_DURATION ||
        spawnEffectTexture.id == 0) {
        return;
    }

    int frame = std::min(
        SPAWN_EFFECT_FRAME_COUNT - 1,
        (int)(spawnSequenceElapsed / SPAWN_EFFECT_FRAME_DURATION)
    );
    Vector2 effectFootAnchor = GetRenderFootPosition();
    Rectangle source = {
        frame * SPAWN_EFFECT_FRAME_WIDTH,
        0.0f,
        SPAWN_EFFECT_FRAME_WIDTH,
        SPAWN_EFFECT_FRAME_HEIGHT
    };
    Rectangle destination = {
        effectFootAnchor.x,
        effectFootAnchor.y,
        SPAWN_EFFECT_FRAME_WIDTH,
        SPAWN_EFFECT_FRAME_HEIGHT
    };
    DrawTexturePro(
        spawnEffectTexture,
        source,
        destination,
        {
            SPAWN_EFFECT_FRAME_WIDTH * 0.5f,
            SPAWN_EFFECT_FRAME_HEIGHT
        },
        0.0f,
        WHITE
    );
}

Rectangle Enemy::GetBoundingBox() const {
    return { position.x - size.x/2.f, position.y - size.y/2.f, size.x, size.y };
}

Rectangle Enemy::GetCollisionBox() const {
    return GetNavigationFootprintAt(position);
}

Rectangle Enemy::GetNavigationFootprintAt(Vector2 entityPosition) const {
    return {
        entityPosition.x + collisionProfile.navigationCenterOffset.x -
            collisionProfile.navigationSize.x / 2.0f,
        entityPosition.y + collisionProfile.navigationCenterOffset.y -
            collisionProfile.navigationSize.y / 2.0f,
        collisionProfile.navigationSize.x,
        collisionProfile.navigationSize.y
    };
}

Rectangle Enemy::GetContactAttackBoxAt(Vector2 entityPosition) const {
    return {
        entityPosition.x - size.x / 2.0f,
        entityPosition.y - size.y / 2.0f,
        size.x,
        size.y
    };
}

void Enemy::DrawPathDebug() const {
    if (!Constants::DEBUG_DRAW_ENEMY_PATHS || health <= 0) {
        return;
    }

    for (const EnemyPathDebugPoint& point : pathDebugPoints) {
        DrawCircleV(
            point.position,
            2.5f,
            point.hasLineOfSight ? GREEN : RED
        );
    }

    if (hasSelectedPathGoal) {
        DrawCircleLines(
            (int)selectedPathGoal.x,
            (int)selectedPathGoal.y,
            5.0f,
            LIME
        );
    }

    if (!usePathFinding) return;

    Vector2 segmentStart = position;
    bool isCurrentTarget = true;
    for (Vector2 targetPosition : targetPositions) {
        DrawLineEx(
            segmentStart,
            targetPosition,
            DEBUG_PATH_THICKNESS,
            DEBUG_PATH_COLOR
        );
        DrawCircleV(
            targetPosition,
            isCurrentTarget
                ? DEBUG_CURRENT_TARGET_RADIUS
                : DEBUG_WAYPOINT_RADIUS,
            isCurrentTarget
                ? DEBUG_CURRENT_TARGET_COLOR
                : DEBUG_WAYPOINT_COLOR
        );

        segmentStart = targetPosition;
        isCurrentTarget = false;
    }

    Vector2 statusPosition = {
        position.x,
        position.y - DEBUG_STATUS_OFFSET_Y
    };
    if (pathStatus == EnemyPathStatus::Pending) {
        DrawCircleV(statusPosition, DEBUG_WAYPOINT_RADIUS, DEBUG_PENDING_COLOR);
    } else if (pathStatus == EnemyPathStatus::Unreachable) {
        constexpr float CROSS_RADIUS = 5.0f;
        DrawLineEx(
            { statusPosition.x - CROSS_RADIUS, statusPosition.y - CROSS_RADIUS },
            { statusPosition.x + CROSS_RADIUS, statusPosition.y + CROSS_RADIUS },
            DEBUG_PATH_THICKNESS,
            DEBUG_UNREACHABLE_COLOR
        );
        DrawLineEx(
            { statusPosition.x - CROSS_RADIUS, statusPosition.y + CROSS_RADIUS },
            { statusPosition.x + CROSS_RADIUS, statusPosition.y - CROSS_RADIUS },
            DEBUG_PATH_THICKNESS,
            DEBUG_UNREACHABLE_COLOR
        );
    } else if (pathStatus == EnemyPathStatus::AtGoal) {
        DrawCircleLines(
            (int)statusPosition.x,
            (int)statusPosition.y,
            DEBUG_CURRENT_TARGET_RADIUS,
            DEBUG_CURRENT_TARGET_COLOR
        );
    }
}

bool Enemy::CheckCollision(const std::vector<GameObject*>& entities) const {
    return EnemyCollision::CheckAnyEnemyCollision(*this, entities);
}

void Enemy::StartPathFinding() {
    if (usePathFinding) return;

    usePathFinding = true;
    pathStatus = EnemyPathStatus::Pending;
    pathAccess.BeginPathFinding(*this);
}

void Enemy::StartPathFindingTo(Vector2 worldGoal) {
    if (usePathFinding) {
        EndPathFinding();
    }

    usePathFinding = true;
    pathStatus = EnemyPathStatus::Pending;
    pathAccess.BeginPathFindingTo(*this, worldGoal);
}

void Enemy::EndPathFinding() {
    if (!usePathFinding) return;

    pathAccess.EndPathFinding(*this);
    usePathFinding = false;
    ClearTargetPosition();
    pathStatus = EnemyPathStatus::Pending;
}

void Enemy::ApplyKnockback(Vector2 dir, float force) {
    float resistedForce = std::max(0.0f, force) * GetKnockbackMultiplier();
    if (resistedForce <= 0.0f) return;

    knockbackVelocity.x += dir.x * resistedForce;
    knockbackVelocity.y += dir.y * resistedForce;
}

void Enemy::ApplyCollisionPush(Vector2 dir, float distance) {
    float resistedDistance = std::max(0.0f, distance) *
        GetKnockbackMultiplier();
    if (resistedDistance <= 0.0f) return;

    EnemyCollision::MoveAgainstWalls(
        *this,
        Vector2Scale(dir, resistedDistance),
        pathAccess,
        EnemyWallResponse::Slide
    );
}

void Enemy::SetKnockbackResistance(float resistance) {
    knockbackResistance = std::clamp(resistance, 0.0f, 1.0f);
}

void Enemy::UpdateKnockback(float deltaTime) {
    if (Vector2Length(knockbackVelocity) > 5.0f) {
        knockbackVelocity.x -= knockbackVelocity.x * 15.0f * deltaTime;
        knockbackVelocity.y -= knockbackVelocity.y * 15.0f * deltaTime;
        
        EnemyMoveResult moveResult = EnemyCollision::MoveAgainstWalls(
            *this,
            Vector2Scale(knockbackVelocity, deltaTime),
            pathAccess,
            EnemyWallResponse::Slide
        );

        if (moveResult.blockedX) {
            knockbackVelocity.x = 0.0f;
        }

        if (moveResult.blockedY) {
            knockbackVelocity.y = 0.0f;
        }
    } else {
        knockbackVelocity = {0.0f, 0.0f};
    }
}


EnemyMoveResult Enemy::UpdateMovement(Vector2 desiredVelocity, float deltaTime, EnemyWallResponse response) {
    if (statusComponent.HasEffect(EffectType::SLOW)) {
        desiredVelocity.x *= 0.5f;
        desiredVelocity.y *= 0.5f;
    }

    float friction = 6.0f;
    currentVelocity.x += (desiredVelocity.x - currentVelocity.x) * friction * deltaTime;
    currentVelocity.y += (desiredVelocity.y - currentVelocity.y) * friction * deltaTime;
    
    Vector2 displacement = { currentVelocity.x * deltaTime, currentVelocity.y * deltaTime };
    return EnemyCollision::MoveAgainstWalls(*this, displacement, pathAccess, response);
}

void Enemy::ApplyStatMultiplier(float multiplier) {
    maxHealth = (int)(maxHealth * multiplier);
    health = maxHealth;
    damage = (int)(damage * multiplier);
    // Slight speed buff (half of the multiplier scale)
    speed *= (1.0f + (multiplier - 1.0f) * 0.5f);
}
