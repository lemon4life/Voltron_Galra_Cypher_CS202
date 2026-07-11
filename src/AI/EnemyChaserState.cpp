#include "AI/EnemyState.h"
#include "Entities/EnemyEntities/Chaser.h"
#include "Entities/Player/Player.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "raymath.h"

namespace {
    bool IsBlocked(LevelManager* levelManager, Enemy* enemy) {
        return levelManager && levelManager->IsSolidCollision(enemy->GetBoundingBox());
    }
}

void EnemyChaserChaseState::Enter(Enemy* enemy) { }

void EnemyChaserChaseState::Update(Enemy* enemy, float deltaTime) {
    EnemyChaser* chaser = dynamic_cast<EnemyChaser*>(enemy);
    if (!chaser || !enemy->GetTarget()) return;

    
    Vector2 ePos = enemy->GetPosition();
    Vector2 pPos = enemy->GetTarget()->GetPosition();
    LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
    Vector2 dir = { 0.0f, 0.0f };
    float speed = enemy->GetSpeed();
    float distanceToP = Vector2Length(Vector2Subtract(pPos, ePos));
    
    // Change back to idleState if player is too far away
    if (Vector2Distance(ePos, pPos) > offSightDistance) {
        chaser->EndPathFinding();
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }
    
    // Apply Path finding if far from player
    // If near player use head to player direction directly
    if (distanceToP > 10.f) {
        chaser->StartPathFinding();
        Vector2 moveTarget = pPos;
        if (levelManager) {
            moveTarget = levelManager->GetEnemyPathManager().GetNextMoveTarget(levelManager, enemy, pPos);
        }
        dir = Vector2Subtract(moveTarget, ePos);
    } else {
        chaser->EndPathFinding();
        dir = Vector2Subtract(pPos, ePos);
    }

    if (Vector2Length(dir) > 0.0f) {
        dir = Vector2Normalize(dir);
    }

    if (levelManager) {
        dir = levelManager->GetEnemyPathManager().GetLocalAvoidanceDirection(levelManager, enemy, dir);
    }

    // Collision calculation
    {
        ePos = enemy->GetPosition();

        // X Axis Wall Sliding
        ePos.x += dir.x * speed * deltaTime;
        enemy->SetPosition(ePos);
        if (IsBlocked(levelManager, enemy)) {
            ePos.x -= dir.x * speed * deltaTime;
            enemy->SetPosition(ePos);
        }
        ePos = enemy->GetPosition();

        // Y Axis Wall Sliding
        ePos.y += dir.y * speed * deltaTime;
        enemy->SetPosition(ePos);
        if (IsBlocked(levelManager, enemy)) {
            ePos.y -= dir.y * speed * deltaTime;
            enemy->SetPosition(ePos);
        }
        ePos = enemy->GetPosition();

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
                enemy->SetAttackCooldown(enemy->GetAttackCooldown());
            }
            
            // Separation knockback (push enemy away from player to prevent freeze/deadlock)
            Vector2 pushDir = Vector2Subtract(ePos, pPos);
            if (Vector2Length(pushDir) == 0.0f) pushDir = {1.0f, 0.0f}; // Fallback if exactly on top
            pushDir = Vector2Normalize(pushDir);
            
            Vector2 beforePush = ePos;
            ePos.x += pushDir.x * 20.0f;
            enemy->SetPosition(ePos);
            if (IsBlocked(levelManager, enemy)) {
                ePos.x = beforePush.x;
                enemy->SetPosition(ePos);
            }

            beforePush = ePos;
            ePos.y += pushDir.y * 20.0f;
            enemy->SetPosition(ePos);
            if (IsBlocked(levelManager, enemy)) {
                ePos.y = beforePush.y;
                enemy->SetPosition(ePos);
            }
        }
    }
}

void EnemyChaserChaseState::Exit(Enemy* enemy) {
    // Exit from path finding manager if it is still on
    if (EnemyChaser* chaser = dynamic_cast<EnemyChaser*>(enemy)) {
        chaser->EndPathFinding();
    }
}
