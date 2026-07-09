#include "Entities/EnemyEntities/Chaser.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/EnemyPathManager.h"
#include "Core/Manager/GameManager.h"

EnemyChaser::EnemyChaser(Vector2 pos, Player* target)
    : Enemy(pos, target, MAX_HEALTH, BASE_SPEED, BASE_DAMAGE, BASE_ATTACK_COOLDOWN)
{
    idleState = std::make_unique<EnemyIdleState>();
    chaseState = std::make_unique<EnemyChaserChaseState>();
    enemyType = EnemyType::Chaser;

    size = (Vector2){ WIDTH, HEIGHT };;

    idleState->UpdateDistance(SIGHT);
    chaseState->UpdateDistance(SIGHT);
    currentState = idleState.get();
    currentState->Enter(this);
}

EnemyChaser::~EnemyChaser() {
    EndPathFinding();

    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void EnemyChaser::Update(float deltaTime) {
    if (currentState) {
        currentState->Update(this, deltaTime);
    }
}

void EnemyChaser::Draw() {

    // Different state gives different color
    if (currentState == chaseState.get()) {
        DrawRectangleRec(GetBoundingBox(), MAROON);
    }
    else {
        DrawRectangleRec(GetBoundingBox(), LIME);
    }

    float hpPercent = (float) health / (float) maxHealth;
    DrawRectangle(position.x - size.x / 2.f, position.y - 20, size.x * hpPercent, 4, RED);
}

void EnemyChaser::StartPathFinding() {
    if (IsPathFinding()) return;
    SetPathFinding(true);

    for (IEnemyObserver* observer : observers) {
        observer->OnEnemyPathFind(this);
    }
}

void EnemyChaser::EndPathFinding() {
    if (!IsPathFinding()) return;
    SetPathFinding(false);
    ClearTargetPosition();

    for (IEnemyObserver* observer : observers) {
        observer->OnEnemyPathFindEnded(this);
    }
}
