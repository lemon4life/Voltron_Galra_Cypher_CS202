#include "Entities/EnemyEntities/EnemyRange.h"

#include "AI/EnemyState.h"

#include "raymath.h"

EnemyRange::EnemyRange(Vector2 position, TeamManager* targetTeam)
    : Enemy(
          position,
          targetTeam,
          RANGE_MAX_HEALTH,
          RANGE_SPEED,
          RANGE_DAMAGE,
          RANGE_ATTACK_COOLDOWN
      ) {
    idleState = std::make_unique<EnemyIdleState>(RANGE_DETECTION_DISTANCE);
    chaseState = std::make_unique<EnemyRangeChaseState>();
    shootingState = std::make_unique<EnemyRangeShootingState>();
    enemyType = EnemyType::RANGE;
    size = RANGE_SIZE;

    ChangeState(GetIdleState());
}

EnemyRange::~EnemyRange() {
    EndPathFinding();

    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void EnemyRange::Update(float deltaTime) {
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
    return MAX_PREDICTION_TIME;
}

void EnemyRange::StartPathFinding() {
    if (IsPathFinding()) return;
    SetPathFinding(true);

    for (IEnemyObserver* observer : observers) {
        observer->OnEnemyPathFind(this);
    }
}

void EnemyRange::EndPathFinding() {
    if (!IsPathFinding()) return;
    SetPathFinding(false);
    ClearTargetPosition();

    for (IEnemyObserver* observer : observers) {
        observer->OnEnemyPathFindEnded(this);
    }
}
