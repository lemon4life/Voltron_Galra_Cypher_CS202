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

    bool TryMoveToClearPosition(LevelManager* levelManager, Enemy* enemy, Vector2 position) {
        Vector2 oldPosition = enemy->GetPosition();
        enemy->SetPosition(position);

        if (IsBlocked(levelManager, enemy)) {
            enemy->SetPosition(oldPosition);
            return false;
        }

        return true;
    }

    bool ResolveWallOverlap(LevelManager* levelManager, Enemy* enemy) {
        if (!IsBlocked(levelManager, enemy)) {
            return true;
        }

        Vector2 origin = enemy->GetPosition();
        const float step = 2.0f;
        const float maxDistance = 64.0f;

        for (float distance = step; distance <= maxDistance; distance += step) {
            const Vector2 candidates[8] = {
                { origin.x + distance, origin.y },
                { origin.x - distance, origin.y },
                { origin.x, origin.y + distance },
                { origin.x, origin.y - distance },
                { origin.x + distance, origin.y + distance },
                { origin.x + distance, origin.y - distance },
                { origin.x - distance, origin.y + distance },
                { origin.x - distance, origin.y - distance }
            };

            for (Vector2 candidate : candidates) {
                if (TryMoveToClearPosition(levelManager, enemy, candidate)) {
                    return true;
                }
            }
        }

        enemy->SetPosition(origin);
        return false;
    }
}

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
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }
    
    // Apply Path finding if far from player
    // If near player use head to player direction directly
    if (distanceToP > 20.f) {
        chaser->StartPathFinding();
        if (chaser->HasTargetPosition()) {
            while (chaser->HasTargetPosition() &&
                   Vector2Distance(chaser->FirstTargetPosition(), ePos) <= 4.0f) {
                chaser->PopTarget();
            }

            if (chaser->HasTargetPosition()) {
                Vector2 pathTarget = chaser->FirstTargetPosition();
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
        LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();

        ResolveWallOverlap(levelManager, enemy);
        ePos = enemy->GetPosition();

        // X Axis Wall Sliding
        ePos.x += dir.x * speed * deltaTime;
        enemy->SetPosition(ePos);
        if (IsBlocked(levelManager, enemy)) {
            ePos.x -= dir.x * speed * deltaTime;
            enemy->SetPosition(ePos);
        }
        ResolveWallOverlap(levelManager, enemy);
        ePos = enemy->GetPosition();

        // Y Axis Wall Sliding
        ePos.y += dir.y * speed * deltaTime;
        enemy->SetPosition(ePos);
        if (IsBlocked(levelManager, enemy)) {
            ePos.y -= dir.y * speed * deltaTime;
            enemy->SetPosition(ePos);
        }
        ResolveWallOverlap(levelManager, enemy);
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
                enemy->SetAttackCooldown(1.0f); // 1 second cooldown
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

            ResolveWallOverlap(levelManager, enemy);
        }
    }
}

void EnemyChaserChaseState::Exit(Enemy* enemy) {
    // Exit from path finding manager if it is still on
    if (EnemyChaser* chaser = dynamic_cast<EnemyChaser*>(enemy)) {
        chaser->EndPathFinding();
    }
}
