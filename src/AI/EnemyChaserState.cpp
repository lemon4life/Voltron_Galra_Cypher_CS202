#include "AI/EnemyState.h"
#include "Entities/EnemyEntities/Chaser.h"
#include "Entities/Player.h"
#include "Core/GameManager.h"
#include "Entities/Wall.h"
#include "raymath.h"

void EnemyChaserChaseState::Enter(Enemy* enemy) { }

void EnemyChaserChaseState::Update(Enemy* enemy, float deltaTime) {
    EnemyChaser* chaser = dynamic_cast<EnemyChaser*>(enemy);
    if (!chaser || !enemy->GetTarget()) return;

    
    Vector2 ePos = enemy->GetPosition();
    Vector2 pPos = enemy->GetTarget()->GetPosition();
    Vector2 dir;
    float speed = enemy->GetSpeed();
    float distanceToP = Vector2Length(Vector2Subtract(pPos, ePos));
    
    // Change back to idleState if player is too far away
    if (Vector2Distance(ePos, pPos) > offSightDistance) {
        chaser->EndPathFinding();
        enemy->ChangeState(chaser->GetChaserIdleState());
        return;
    }
    
    // Apply Path finding if far from player
    // If near player use head to player direction directly
    if (distanceToP > 20.f) {
        chaser->StartPathFinding();
        if (chaser->HasTargetPosition()) {
            Vector2 pathTarget = chaser->GetTargetPosition();
            if (Vector2Distance(pathTarget, ePos) > 4.0f) {
                dir = Vector2Subtract(pathTarget, ePos);
            } else {
                dir = Vector2Subtract(pPos, ePos);
            }
        } else {
            dir = Vector2Subtract(pPos, ePos);
        }
    } else {
        chaser->EndPathFinding();
        dir = Vector2Subtract(pPos, ePos);
    }

    if (Vector2Length(dir) > 0.0f) {
        dir = Vector2Normalize(dir);
    }

    {   
        const auto& entities = GameManager::GetInstance().GetLevelEntities();

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
            ePos.x -= dir.x * speed * deltaTime;
            enemy->SetPosition(ePos);
        }

        // Y Axis Wall Sliding
        ePos.y += dir.y * speed * deltaTime;
        enemy->SetPosition(ePos);
        if (enemy->CheckCollision(walls)) {
            ePos.y -= dir.y * speed * deltaTime;
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
}

void EnemyChaserChaseState::Exit(Enemy* enemy) {
    // Exit from path finding manager if it is still on
    if (EnemyChaser* chaser = dynamic_cast<EnemyChaser*>(enemy)) {
        chaser->EndPathFinding();
    }
}
