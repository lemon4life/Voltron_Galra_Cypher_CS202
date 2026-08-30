#include "AI/EnemyState.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"
#include "raymath.h"

#include <algorithm>

// --- EnemyIdleState ---
/// Creates a EnemyIdleState instance from the supplied configuration.
EnemyIdleState::EnemyIdleState(float spotDistance)
    : spotDistance(spotDistance) {}

/// Prepares this state when it becomes active.
void EnemyIdleState::Enter(Enemy* enemy) {}

/// Advances this component's state for the current frame.
void EnemyIdleState::Update(Enemy* enemy, float deltaTime) {
    if (!enemy->GetTargetTeam()) return;
    Paladin* activePaladin = enemy->GetTargetTeam()->GetActivePaladin();
    if (!activePaladin) return;

    Vector2 ePos = enemy->GetPosition();
    Vector2 pPos = activePaladin->GetPosition();
    
    if (Vector2Distance(ePos, pPos) < spotDistance) {
        enemy->ChangeState(enemy->GetChaseState());
    }
}

/// Cleans up this state before control moves elsewhere.
void EnemyIdleState::Exit(Enemy* enemy) {}

// --- EnemyDazeState ---
/// Prepares this state when it becomes active.
void EnemyDazeState::Enter(Enemy* enemy) {
    dTimer = std::max(0.0f, enemy->GetDazeDuration());
}

/// Advances this component's state for the current frame.
void EnemyDazeState::Update(Enemy* enemy, float deltaTime) {
    dTimer = std::max(0.0f, dTimer - deltaTime);
    if (dTimer <= 0.0f) {
        enemy->ChangeState(enemy->GetChaseState());
    }
}

/// Cleans up this state before control moves elsewhere.
void EnemyDazeState::Exit(Enemy* enemy) {
    dTimer = 0.0f;
}
