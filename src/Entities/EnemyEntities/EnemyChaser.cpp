#include "Entities/EnemyEntities/EnemyChaser.h"

#include "raymath.h"

namespace {
    constexpr int CHASER_MAX_HEALTH = 80;
    constexpr float CHASER_SPEED = 170.0f;
    constexpr int CHASER_DAMAGE = 15;
    constexpr float CHASER_ATTACK_COOLDOWN = 1.f;
    constexpr float CHASER_SIGHT_DISTANCE = 40000.0f;
    constexpr float CHASER_STOP_PATH_FINDING_DISTANCE = 30.0f;
    constexpr float CHASER_DAMAGE_CHARGE_DISTANCE = 64.0f;
    constexpr float CHASER_DAMAGE_CHARGE_DURATION = 0.16f;
    constexpr Vector2 CHASER_SIZE = { 20.0f, 20.0f };
}

EnemyChaser::EnemyChaser(
    Vector2 position,
    TeamManager* targetTeam,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess
)
    : Enemy(
          position,
          targetTeam,
          CHASER_MAX_HEALTH,
          CHASER_SPEED,
          CHASER_DAMAGE,
          CHASER_ATTACK_COOLDOWN,
          removalAccess,
          pathAccess
      ) {
    idleState = std::make_unique<EnemyIdleState>(CHASER_SIGHT_DISTANCE);
    chaseState = std::make_unique<EnemyChaserChaseState>();
    damageState = std::make_unique<EnemyChaserDamageState>();
    enemyType = EnemyType::Chaser;
    size = CHASER_SIZE;

    ChangeState(GetIdleState());
}

EnemyChaser::~EnemyChaser() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void EnemyChaser::Update(float deltaTime) {
    UpdateKnockback(deltaTime);
    if (currentState) {
        currentState->Update(this, deltaTime);
    }
}

void EnemyChaser::Draw() {
    if (currentState == chaseState.get()) {
        DrawRectangleRec(GetBoundingBox(), MAROON);
    } else {
        DrawRectangleRec(GetBoundingBox(), LIME);
    }

    float healthPercent = (float)health / (float)maxHealth;
    DrawRectangle(
        (int)(position.x - size.x / 2.0f),
        (int)(position.y - 20.0f),
        (int)(size.x * healthPercent),
        4,
        RED
    );
}

EnemyChaserDamageState* EnemyChaser::GetDamageState() {
    return damageState.get();
}

bool EnemyChaser::IsBeyondDisengageDistance(Vector2 targetPosition) const {
    return Vector2Distance(position, targetPosition) > CHASER_SIGHT_DISTANCE;
}

bool EnemyChaser::IsWithinStopPathFindingDistance(
    Vector2 targetPosition
) const {
    return Vector2Distance(position, targetPosition) <=
           CHASER_STOP_PATH_FINDING_DISTANCE;
}

float EnemyChaser::GetDamageChargeDistance() const {
    return CHASER_DAMAGE_CHARGE_DISTANCE;
}

float EnemyChaser::GetDamageChargeDuration() const {
    return CHASER_DAMAGE_CHARGE_DURATION;
}
