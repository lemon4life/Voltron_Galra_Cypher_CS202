#include "AI/EnemyState.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/GameManager.h"
#include "Entities/Wall.h"
#include "Core/Manager/LevelManager.h"
#include "raymath.h"

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

// --- EnemyChaseState ---
EnemyChaseState::EnemyChaseState(float offSightDistance)
    : offSightDistance(offSightDistance) {}

void EnemyChaseState::Enter(Enemy* enemy) {}

void EnemyChaseState::Update(Enemy* enemy, float deltaTime) {
    if (!enemy->GetTargetTeam()) return;

    Vector2 ePos = enemy->GetPosition();
    Vector2 pPos = enemy->GetTargetTeam()->GetActivePaladin()->GetPosition();
    
    if (Vector2Distance(ePos, pPos) > offSightDistance) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    // Direct Vector Math
    Vector2 dir = Vector2Subtract(pPos, ePos);
    if (Vector2Length(dir) > 0.0f) {
        dir = Vector2Normalize(dir);
    }
    
    float speed = enemy->GetSpeed();
    LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
    float levelWidth = GameManager::GetInstance().GetLevelWidth();
    float levelHeight = GameManager::GetInstance().GetLevelHeight();

    // X Axis Wall Sliding
    ePos.x += dir.x * speed * deltaTime;
    // Bounds check
    if (ePos.x < 0.0f) ePos.x = 0.0f;
    if (ePos.x > levelWidth) ePos.x = levelWidth;
    
    enemy->SetPosition(ePos);
    if (levelManager && levelManager->IsSolidCollision(enemy->GetBoundingBox())) {
        ePos.x -= dir.x * speed * deltaTime; // Revert X
        enemy->SetPosition(ePos);
    }

    // Y Axis Wall Sliding
    ePos.y += dir.y * speed * deltaTime;
    // Bounds check
    if (ePos.y < 0.0f) ePos.y = 0.0f;
    if (ePos.y > levelHeight) ePos.y = levelHeight;
    
    enemy->SetPosition(ePos);
    if (levelManager && levelManager->IsSolidCollision(enemy->GetBoundingBox())) {
        ePos.y -= dir.y * speed * deltaTime; // Revert Y
        enemy->SetPosition(ePos);
    }

    // Handle Attack Cooldown
    if (enemy->GetAttackCooldown() > 0.0f) {
        float remainingCooldown = enemy->GetAttackCooldown() - deltaTime;
        enemy->SetAttackCooldown(remainingCooldown > 0.0f ? remainingCooldown : 0.0f);
    }
    
    // Check collision with Player for overlap resolution and damage
    if (CheckCollisionRecs(enemy->GetBoundingBox(), enemy->GetTargetTeam()->GetActivePaladin()->GetBoundingBox())) {
        Paladin* activePaladin = enemy->GetTargetTeam()->GetActivePaladin();
        if (activePaladin->CanParryAttack(ePos)) {
            activePaladin->TriggerParrySuccess(enemy);
            activePaladin->IncrementParryCount();
            GameManager::GetInstance().TriggerHitstop(0.3f);
            GameManager::GetInstance().AddImpactEffect({pPos.x, pPos.y}); // Visual spark on block!
            enemy->SetAttackCooldown(2.0f); // Stun
            
            // Massive pushback
            Vector2 pushDir = Vector2Subtract(ePos, pPos);
            if (Vector2Length(pushDir) == 0.0f) pushDir = {1.0f, 0.0f};
            pushDir = Vector2Normalize(pushDir);
            ePos.x += pushDir.x * 60.0f;
            ePos.y += pushDir.y * 60.0f;
            enemy->SetPosition(ePos);
        } else if (enemy->GetAttackCooldown() <= 0.0f) {
            activePaladin->TakeDamage(enemy->GetDamage());
            enemy->ResetAttackCooldown();
            
            // Break parry state if they took damage (limit exceeded)
            if (activePaladin->IsParrying()) {
                activePaladin->ChangeState(activePaladin->GetIdleState());
            }
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
