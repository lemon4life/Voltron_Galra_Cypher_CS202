#include "AI/EnemyState.h"
#include "Entities/Enemy.h"
#include "Entities/Player.h"
#include "Core/GameManager.h"
#include "Entities/Wall.h"
#include "raymath.h"

// --- EnemyIdleState ---
void EnemyIdleState::Enter(Enemy* enemy) {}

void EnemyIdleState::Update(Enemy* enemy, float deltaTime) {
    if (!enemy->GetTarget()) return;

    Vector2 ePos = enemy->GetPosition();
    Vector2 pPos = enemy->GetTarget()->GetPosition();
    
    if (Vector2Distance(ePos, pPos) < 200.0f) {
        enemy->ChangeState(enemy->GetChaseState());
    }
}

void EnemyIdleState::Exit(Enemy* enemy) {}

// --- EnemyChaseState ---
void EnemyChaseState::Enter(Enemy* enemy) {}

void EnemyChaseState::Update(Enemy* enemy, float deltaTime) {
    if (!enemy->GetTarget()) return;

    Vector2 ePos = enemy->GetPosition();
    Vector2 pPos = enemy->GetTarget()->GetPosition();
    
    if (Vector2Distance(ePos, pPos) > 300.0f) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    // Direct Vector Math
    Vector2 dir = Vector2Subtract(pPos, ePos);
    if (Vector2Length(dir) > 0.0f) {
        dir = Vector2Normalize(dir);
    }
    
    float speed = enemy->GetSpeed();
    const auto& entities = GameManager::GetInstance().GetLevelEntities();

    // Filter walls only for collision to prevent enemies blocking each other completely
    std::vector<GameObject*> walls;
    for (auto* e : entities) {
        if (dynamic_cast<Wall*>(e)) {
            walls.push_back(e);
        }
    }

    // X Axis Wall Sliding
    ePos.x += dir.x * speed * deltaTime;
    enemy->SetPosition(ePos);
    if (enemy->CheckCollision(walls)) {
        ePos.x -= dir.x * speed * deltaTime; // Revert X
        enemy->SetPosition(ePos);
    }

    // Y Axis Wall Sliding
    ePos.y += dir.y * speed * deltaTime;
    enemy->SetPosition(ePos);
    if (enemy->CheckCollision(walls)) {
        ePos.y -= dir.y * speed * deltaTime; // Revert Y
        enemy->SetPosition(ePos);
    }

    // Handle Attack Cooldown
    float cd = enemy->GetAttackCooldown();
    if (cd > 0.0f) {
        enemy->SetAttackCooldown(cd - deltaTime);
    }

    // Check collision with Player to deal damage
    if (enemy->GetAttackCooldown() <= 0.0f) {
        if (CheckCollisionRecs(enemy->GetBoundingBox(), enemy->GetTarget()->GetBoundingBox())) {
            enemy->GetTarget()->TakeDamage(enemy->GetDamage());
            enemy->SetAttackCooldown(1.0f); // 1 second cooldown between attacks
        }
    }
}

void EnemyChaseState::Exit(Enemy* enemy) {}
