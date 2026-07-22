#include "AI/EnemyState.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"
#include "raymath.h"

#include <algorithm>

// --- EnemyIdleState ---
EnemyIdleState::EnemyIdleState(float spotDistance)
    : spotDistance(spotDistance) {}

void EnemyIdleState::Enter(Enemy* enemy) {}

void EnemyIdleState::Update(Enemy* enemy, float deltaTime) {
    if (!enemy->GetTargetTeam()) return;

    Vector2 ePos = enemy->GetPosition();
    Vector2 pPos = enemy->GetTargetTeam()->GetActivePaladin()->GetPosition();
    
    if (Vector2Distance(ePos, pPos) < spotDistance) {
        enemy->ChangeState(enemy->GetChaseState());
    }
}

void EnemyIdleState::Exit(Enemy* enemy) {}

// --- EnemyDazeState ---
void EnemyDazeState::Enter(Enemy* enemy) {
    dTimer = std::max(0.0f, enemy->GetDazeDuration());
}

void EnemyDazeState::Update(Enemy* enemy, float deltaTime) {
    dTimer = std::max(0.0f, dTimer - deltaTime);
    if (dTimer <= 0.0f) {
        enemy->ChangeState(enemy->GetChaseState());
    }
}

void EnemyDazeState::Exit(Enemy* enemy) {
    dTimer = 0.0f;
}
