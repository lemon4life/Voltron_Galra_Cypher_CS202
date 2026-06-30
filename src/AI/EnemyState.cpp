#include "AI/EnemyState.h"
#include "Entities/Enemy.h"
#include "Entities/Player.h"
#include "Core/GameManager.h"
#include "Core/LevelManager.h"
#include "Entities/Projectile.h"
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
        enemy->SetAttackCooldown(enemy->GetAttackCooldown() - deltaTime);
    }
    
    if (enemy->GetType() == EnemyType::BOSS) {
        float skillCd = enemy->GetBossSkillCooldown() - deltaTime;
        enemy->SetBossSkillCooldown(skillCd);
        if (skillCd <= 0.0f) {
            enemy->ChangeState(enemy->GetBossRangedAttackState());
            return;
        }
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

void BossRangedAttackState::Enter(Enemy* enemy) {
    enemy->SetBurstCount(3);
    enemy->SetBurstTimer(0.0f);
}

void BossRangedAttackState::Update(Enemy* enemy, float deltaTime) {
    if (!enemy->GetTarget()) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    float timer = enemy->GetBurstTimer() + deltaTime;
    
    // Fire every 0.15 seconds
    if (timer >= 0.15f) {
        timer = 0.0f;
        int count = enemy->GetBurstCount();
        
        // Fire projectile
        Vector2 pos = enemy->GetPosition();
        Vector2 tPos = enemy->GetTarget()->GetPosition();
        Vector2 dir = Vector2Subtract(tPos, pos);
        if (Vector2Length(dir) > 0.0f) {
            dir = Vector2Normalize(dir);
        } else {
            dir = {1.0f, 0.0f};
        }
        
        Vector2 vel = { dir.x * 300.0f, dir.y * 300.0f };
        Projectile* p = new Projectile(pos, vel, 2.0f, enemy->GetDamage(), true);
        GameManager::GetInstance().AddProjectile(p);
        
        count--;
        enemy->SetBurstCount(count);
        
        if (count <= 0) {
            enemy->SetBossSkillCooldown(2.0f);
            enemy->ChangeState(enemy->GetChaseState());
            return;
        }
    }
    enemy->SetBurstTimer(timer);
}

void BossRangedAttackState::Exit(Enemy* enemy) {
    // Reset any state if needed
}
