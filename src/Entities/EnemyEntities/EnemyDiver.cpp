#include "Entities/EnemyEntities/EnemyDiver.h"

#include "AI/EnemyState.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"

#include "raymath.h"

#include <algorithm>



namespace {
    constexpr int DIVER_MAX_HEALTH = 200;
    constexpr float DIVER_BASE_SPEED = 210.0f;
    constexpr int DIVER_DAMAGE = 70;
    constexpr float DIVER_ATTACK_COOLDOWN = 2.5f;
    constexpr float DIVER_SIGHT_DISTANCE = 40000.0f;
    constexpr float DIVER_OFF_SIGHT_DISTANCE = 40000.0f;
    constexpr float DIVE_STOP_DISTANCE = 70.f;

    constexpr float READY_DURATION = 0.3f;
    constexpr float READY_SPEED = 50.0f;
    constexpr float DIVE_SPEED = 800.0f;
    constexpr float DIVE_DURATION = 0.2f;
    constexpr float DIVE_RECOVERY_DURATION = 0.2f;

    constexpr Vector2 DIVER_SIZE = { 24.0f, 24.0f };
}

EnemyDiver::EnemyDiver(
    Vector2 position,
    TeamManager* targetTeam,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess,
    ILevelLineOfSightQuery& lineOfSightQuery
)
    : Enemy(
          position,
          targetTeam,
          DIVER_MAX_HEALTH,
          DIVER_BASE_SPEED,
          DIVER_DAMAGE,
          DIVER_ATTACK_COOLDOWN,
          removalAccess,
          pathAccess
      ),
      lineOfSightQuery(lineOfSightQuery) {
    idleState = std::make_unique<EnemyIdleState>(DIVER_SIGHT_DISTANCE);
    chaseState = std::make_unique<EnemyDiverChaseState>();
    readyState = std::make_unique<EnemyDiverReadyState>();
    lungingState = std::make_unique<EnemyDiverLungingState>();
    enemyType = EnemyType::DIVER;
    size = DIVER_SIZE;

    ChangeState(GetIdleState());
}

EnemyDiver::~EnemyDiver() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void EnemyDiver::Update(float deltaTime) {
    UpdateKnockback(deltaTime);
    if (currentState) {
        currentState->Update(this, deltaTime);
    }
}

void EnemyDiver::Draw() {
    Color stateColor = LIME;
    if (currentState == chaseState.get()) {
        stateColor = DARKBLUE;
    } else if (currentState == readyState.get()) {
        stateColor = ORANGE;
    } else if (currentState == lungingState.get()) {
        stateColor = RED;
    }

    DrawRectangleRec(GetBoundingBox(), stateColor);

    float healthPercent = (float)health / (float)maxHealth;
    DrawRectangle(
        (int)(position.x - size.x / 2.0f),
        (int)(position.y - size.y / 2.0f - 6.0f),
        (int)(size.x * healthPercent),
        4,
        RED
    );
}

EnemyDiverReadyState* EnemyDiver::GetReadyState() {
    return readyState.get();
}

EnemyDiverLungingState* EnemyDiver::GetLungingState() {
    return lungingState.get();
}

bool EnemyDiver::CanEnterReadyState() const {
    return attackCooldown <= 0.0f && IsWithinClearDiveRange();
}

bool EnemyDiver::IsWithinClearDiveRange() const {
    Paladin* target = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    if (!target) return false;

    if (Vector2Distance(position, target->GetPosition()) > DIVE_STOP_DISTANCE) {
        return false;
    }

    return lineOfSightQuery.HasClearLineOfSight(
        position,
        target->GetPosition(),
        GetCollisionClearanceRadius()
    );
}

bool EnemyDiver::IsBeyondDisengageDistance(Vector2 targetPosition) const {
    return Vector2Distance(position, targetPosition) > DIVER_OFF_SIGHT_DISTANCE;
}

float EnemyDiver::GetReadyDuration() const {
    return READY_DURATION;
}

float EnemyDiver::GetReadySpeed() const {
    return READY_SPEED;
}

float EnemyDiver::GetDiveDuration() const {
    return DIVE_DURATION;
}

float EnemyDiver::GetDiveSpeed() const {
    return DIVE_SPEED;
}

float EnemyDiver::GetDiveStopDistance() const {
    return DIVE_STOP_DISTANCE;
}

float EnemyDiver::GetDiveRecoveryDuration() const {
    return DIVE_RECOVERY_DURATION;
}

float EnemyDiver::GetCollisionClearanceRadius() const {
    return std::max(size.x, size.y) / 2.0f;
}
