#include "Entities/EnemyEntities/Chaser.h"
#include "Core/LevelAccess.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/EnemyPathManager.h"
#include "Core/Manager/GameManager.h"

EnemyChaser::EnemyChaser(
    Vector2 pos,
    TeamManager* targetTeam,
    IEntityRemovalAccess* removalAccess,
    IEnemyPathAccess* pathAccess
)
    : Enemy(
          pos,
          targetTeam,
          MAX_HEALTH,
          BASE_SPEED,
          BASE_DAMAGE,
          BASE_ATTACK_COOLDOWN,
          removalAccess
      ),
      EnemyPathFinding(pathAccess)
{
    idleState = std::make_unique<EnemyIdleState>(SIGHT);
    chaseState = std::make_unique<EnemyChaserChaseState>(SIGHT);
    enemyType = EnemyType::Chaser;

    size = (Vector2){ WIDTH, HEIGHT };;

    ChangeState(GetIdleState());
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

    if (IEnemyPathAccess* pathAccess = GetPathAccess()) {
        pathAccess->BeginPathFinding(this);
    }
}

void EnemyChaser::EndPathFinding() {
    if (!IsPathFinding()) return;
    SetPathFinding(false);
    ClearTargetPosition();

    if (IEnemyPathAccess* pathAccess = GetPathAccess()) {
        pathAccess->EndPathFinding(this);
    }
}
