#include "AI/EnemyState.h"
#include "Entities/EnemyEntities/EnemyChaser.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "raymath.h"

namespace {
    bool IsBlocked(LevelManager* levelManager, Enemy* enemy) {
        return levelManager && levelManager->IsSolidCollision(enemy->GetBoundingBox());
    }
}

EnemyChaserChaseState::EnemyChaserChaseState(float offSightDistance)
    : offSightDistance(offSightDistance) {}

void EnemyChaserChaseState::Enter(EnemyChaser* enemy) { }

void EnemyChaserChaseState::Update(EnemyChaser* enemy, float deltaTime) {
    if (!enemy->GetTargetTeam()) return;

    
    Vector2 ePos = enemy->GetPosition();
    Vector2 pPos = enemy->GetTargetTeam()->GetActivePaladin()->GetPosition();
    LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
    Vector2 dir = { 0.0f, 0.0f };
    float speed = enemy->GetSpeed();
    float distanceToP = Vector2Length(Vector2Subtract(pPos, ePos));
    
    // Change back to idleState if player is too far away
    if (Vector2Distance(ePos, pPos) > offSightDistance) {
        enemy->EndPathFinding();
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }
    
    // Apply Path finding if far from player
    // If near player use head to player direction directly
    if (distanceToP > 10.f) {
        enemy->StartPathFinding();
        Vector2 moveTarget = pPos;
        if (levelManager) {
            moveTarget = levelManager->GetEnemyPathManager().GetNextMoveTarget(levelManager, enemy, pPos);
        }
        dir = Vector2Subtract(moveTarget, ePos);
    } else {
        enemy->EndPathFinding();
        dir = Vector2Subtract(pPos, ePos);
    }

    if (Vector2Length(dir) > 0.0f) {
        dir = Vector2Normalize(dir);
    }

    if (levelManager) {
        dir = levelManager->GetEnemyPathManager().GetLocalAvoidanceDirection(levelManager, enemy, dir);
    }

    // Handle Attack Cooldown
    float cd = enemy->GetAttackCooldown();
    if (cd > 0.0f) {
        float remainingCooldown = cd - deltaTime;
        enemy->SetAttackCooldown(remainingCooldown > 0.0f ? remainingCooldown : 0.0f);
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

        // Check collision with Player for overlap resolution and damage
        if (CheckCollisionRecs(enemy->GetBoundingBox(), enemy->GetTargetTeam()->GetActivePaladin()->GetBoundingBox())) {
            Paladin* activePaladin = enemy->GetTargetTeam()->GetActivePaladin();
            
            if (activePaladin->CanParryAttack(ePos)) {
                activePaladin->TriggerParrySuccess(enemy);
                activePaladin->IncrementParryCount();
                GameManager::GetInstance().TriggerHitstop(0.3f);
                GameManager::GetInstance().AddImpactEffect({pPos.x, pPos.y});
                enemy->SetAttackCooldown(2.0f); // Stun
                
                // Massive pushback
                Vector2 pushDir = Vector2Subtract(ePos, pPos);
                if (Vector2Length(pushDir) == 0.0f) pushDir = {1.0f, 0.0f};
                pushDir = Vector2Normalize(pushDir);
                
                LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
                Vector2 beforePush = ePos;
                
                ePos.x += pushDir.x * 60.0f;
                enemy->SetPosition(ePos);
                if (levelManager && levelManager->IsSolidCollision(enemy->GetBoundingBox())) {
                    ePos.x = beforePush.x;
                    enemy->SetPosition(ePos);
                }
                
                beforePush = ePos;
                ePos.y += pushDir.y * 60.0f;
                enemy->SetPosition(ePos);
                if (levelManager && levelManager->IsSolidCollision(enemy->GetBoundingBox())) {
                    ePos.y = beforePush.y;
                    enemy->SetPosition(ePos);
                }
            } else if (enemy->GetAttackCooldown() <= 0.0f) {
                activePaladin->TakeDamage(enemy->GetDamage());
                enemy->ResetAttackCooldown(); 
                
                if (activePaladin->IsParrying()) {
                    activePaladin->ChangeState(activePaladin->GetIdleState());
                }
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

void EnemyChaserChaseState::Exit(EnemyChaser* enemy) {
    // Exit from path finding manager if it is still on
    enemy->EndPathFinding();
}
