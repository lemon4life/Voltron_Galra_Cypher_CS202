#include "Entities/EnemyEntities/EnemyRange.h"

#include "AI/EnemyState.h"

#include "raymath.h"

namespace {
    constexpr int RANGE_MAX_HEALTH = 70;
    constexpr float RANGE_SPEED = 120.0f;
    constexpr int RANGE_DAMAGE = 12;
    constexpr float RANGE_ATTACK_COOLDOWN = 1.0f;
    constexpr float RANGE_DETECTION_DISTANCE = 700.0f;
    constexpr float RANGE_DISENGAGE_DISTANCE = 900.0f;
    constexpr float RANGE_SHOOTING_DISTANCE = 200.0f;
    constexpr float RANGE_PROJECTILE_SPEED = 320.0f;
    constexpr float RANGE_PROJECTILE_LIFETIME = 2.0f;
    constexpr float RANGE_PROJECTILE_RADIUS = 5.0f;
    constexpr float RANGE_MAX_PREDICTION_TIME = 1.0f;
    constexpr Vector2 RANGE_SIZE = { 24.0f, 24.0f };
}

EnemyRange::EnemyRange(
    Vector2 position,
    TeamManager* targetTeam,
    IEntityRemovalAccess* removalAccess,
    IEnemyPathAccess* pathAccess,
    ILevelLineOfSightQuery* lineOfSightQuery
)
    : PathfindingEnemy(
          position,
          targetTeam,
          RANGE_MAX_HEALTH,
          RANGE_SPEED,
          RANGE_DAMAGE,
          RANGE_ATTACK_COOLDOWN,
          removalAccess,
          pathAccess
      ),
      lineOfSightQuery(lineOfSightQuery) {
    idleState = std::make_unique<EnemyIdleState>(RANGE_DETECTION_DISTANCE);
    chaseState = std::make_unique<EnemyRangeChaseState>();
    shootingState = std::make_unique<EnemyRangeShootingState>();
    enemyType = EnemyType::RANGE;
    size = RANGE_SIZE;

    ChangeState(GetIdleState());
}

EnemyRange::~EnemyRange() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void EnemyRange::Update(float deltaTime) {
    UpdateKnockback(deltaTime);
    if (currentState) {
        currentState->Update(this, deltaTime);
    }
}

void EnemyRange::Draw() {
    DrawRectangleRec(GetBoundingBox(), VIOLET);

    float healthPercent = (float)health / (float)maxHealth;
    DrawRectangle(
        (int)(position.x - size.x / 2.0f),
        (int)(position.y - size.y / 2.0f - 6.0f),
        (int)(size.x * healthPercent),
        4,
        RED
    );
}

EnemyRangeShootingState* EnemyRange::GetShootingState() {
    return shootingState.get();
}

bool EnemyRange::IsWithinShootingDistance(Vector2 targetPosition) const {
    return Vector2Distance(position, targetPosition) <= RANGE_SHOOTING_DISTANCE;
}

bool EnemyRange::IsBeyondDisengageDistance(Vector2 targetPosition) const {
    return Vector2Distance(position, targetPosition) > RANGE_DISENGAGE_DISTANCE;
}

float EnemyRange::GetProjectileSpeed() const {
    return RANGE_PROJECTILE_SPEED;
}

float EnemyRange::GetProjectileLifetime() const {
    return RANGE_PROJECTILE_LIFETIME;
}

float EnemyRange::GetProjectileRadius() const {
    return RANGE_PROJECTILE_RADIUS;
}

float EnemyRange::GetMaxPredictionTime() const {
    return RANGE_MAX_PREDICTION_TIME;
}