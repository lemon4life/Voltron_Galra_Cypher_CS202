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
    
    if (Vector2Distance(ePos, pPos) < 1000.0f) {
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
    
    if (Vector2Distance(ePos, pPos) > 1200.0f) {
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

    // Check collision with Player for overlap resolution and damage
    if (CheckCollisionRecs(enemy->GetBoundingBox(), enemy->GetTarget()->GetBoundingBox())) {
        // Attack if cooldown allows
        if (enemy->GetAttackCooldown() <= 0.0f) {
            enemy->GetTarget()->TakeDamage(enemy->GetDamage());
            enemy->SetAttackCooldown(1.0f); // 1 second cooldown
        }
        
        // Separation knockback (push enemy away from player to prevent freeze/deadlock)
        Vector2 pushDir = Vector2Subtract(ePos, pPos);
        if (Vector2Length(pushDir) == 0.0f) pushDir = {1.0f, 0.0f}; // Fallback if exactly on top
        pushDir = Vector2Normalize(pushDir);
        
        ePos.x += pushDir.x * 20.0f;
        ePos.y += pushDir.y * 20.0f;
        enemy->SetPosition(ePos);
    }
}

void EnemyChaseState::Exit(Enemy* enemy) {}
