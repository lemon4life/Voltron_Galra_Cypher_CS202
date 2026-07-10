#include "AI/EnemyState.h"

#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"

#include "Entities/EnemyEntities/Boss.h"
#include "Entities/Projectile.h"
#include "Entities/Player/Player.h"

#include "raymath.h"

// Boss Chase State

void BossChaseState::Enter(Enemy* enemy) {}

void BossChaseState::Update(Enemy* enemy, float deltaTime) {
    if (!enemy->GetTarget()) return;

    Vector2 ePos = enemy->GetPosition();
    Vector2 pPos = enemy->GetTarget()->GetPosition();
    
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
        enemy->SetAttackCooldown(enemy->GetAttackCooldown() - deltaTime);
    }
    
    if (Boss* boss = dynamic_cast<Boss*>(enemy)) {
        float skillCd = boss->GetBossSkillCooldown() - deltaTime;
        boss->SetBossSkillCooldown(skillCd);
        if (skillCd <= 0.0f) {
            boss->ChangeState(boss->GetBossRangedAttackState());
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

void BossChaseState::Exit(Enemy* enemy) {}

// Boss Ranged Attack State
void BossRangedAttackState::Enter(Enemy* enemy) {
    Boss* boss = dynamic_cast<Boss*>(enemy);
    if (!boss) return;

    boss->SetBurstCount(3);
    boss->SetBurstTimer(0.0f);
}

void BossRangedAttackState::Update(Enemy* enemy, float deltaTime) {
    Boss* boss = dynamic_cast<Boss*>(enemy);
    if (!boss) return;

    if (!enemy->GetTarget()) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    float timer = boss->GetBurstTimer() + deltaTime;
    
    // Fire every 0.15 seconds
    if (timer >= 0.15f) {
        timer = 0.0f;
        int count = boss->GetBurstCount();
        
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
        boss->SetBurstCount(count);
        
        if (count <= 0) {
            boss->SetBossSkillCooldown(2.0f);
            enemy->ChangeState(enemy->GetChaseState());
            return;
        }
    }
    boss->SetBurstTimer(timer);
}

void BossRangedAttackState::Exit(Enemy* enemy) {
    // Reset any state if needed
}
