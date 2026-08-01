#include "AI/EnemyState.h"

#include "AI/EnemyCollision.h"
#include "Core/LevelAccess.h"
#include "Core/Manager/GameManager.h"

#include "Entities/EnemyEntities/Boss.h"
#include "Entities/Projectile.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"

#include "raymath.h"

// Boss Chase State

void BossChaseState::Enter(Boss* enemy) {
    enemy->StartPathFinding();
}

void BossChaseState::Update(Boss* enemy, float deltaTime) {
    if (!enemy->GetTargetTeam()) return;

    Vector2 ePos = enemy->GetPosition();
    Vector2 pPos = enemy->GetTargetTeam()->GetActivePaladin()->GetPosition();
    
    if (enemy->IsBeyondDisengageDistance(pPos)) {
        enemy->ChangeState(enemy->GetIdleState());
        return;
    }

    IEnemyPathAccess& pathAccess = enemy->GetPathAccess();
    std::optional<Vector2> moveTarget =
        pathAccess.GetNextMoveTarget(*enemy);
    Vector2 dir = { 0.0f, 0.0f };
    if (moveTarget) {
        dir = Vector2Subtract(*moveTarget, ePos);
        if (Vector2Length(dir) > 0.0f) {
            dir = Vector2Normalize(dir);
        }
        dir = pathAccess.GetLocalDirection(*enemy, dir);
    }

    EnemyCollision::MoveAgainstWalls(
        *enemy,
        Vector2Scale(dir, enemy->GetSpeed() * deltaTime),
        pathAccess,
        EnemyWallResponse::Slide
    );

    // Handle Attack Cooldown
    if (enemy->GetAttackCooldown() > 0.0f) {
        float remainingCooldown = enemy->GetAttackCooldown() - deltaTime;
        enemy->SetAttackCooldown(remainingCooldown > 0.0f ? remainingCooldown : 0.0f);
    }
    
    float skillCd = enemy->GetBossSkillCooldown() - deltaTime;
    enemy->SetBossSkillCooldown(skillCd);
    if (skillCd <= 0.0f) {
        enemy->ChangeState(enemy->GetBossRangedAttackState());
        return;
    }

    // Check collision with Player for overlap resolution and damage
    Paladin* activePaladin = enemy->GetTargetTeam()->GetActivePaladin();
    if (EnemyCollision::CheckPlayerCollision(*enemy, *activePaladin)) {
        // Attack if cooldown allows
        if (enemy->GetAttackCooldown() <= 0.0f) {
            activePaladin->TakeDamage(enemy->GetDamage());
            enemy->ResetAttackCooldown();
        }
        
        // Separation knockback (push enemy away from player to prevent freeze/deadlock)
        ePos = enemy->GetPosition();
        Vector2 pushDir = Vector2Subtract(ePos, pPos);
        if (Vector2Length(pushDir) == 0.0f) pushDir = {1.0f, 0.0f}; // Fallback if exactly on top
        pushDir = Vector2Normalize(pushDir);

        EnemyCollision::MoveAgainstWalls(
            *enemy,
            Vector2Scale(pushDir, 20.0f),
            pathAccess,
            EnemyWallResponse::Slide
        );
    }
}

void BossChaseState::Exit(Boss* enemy) {
    enemy->EndPathFinding();
}

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

    if (!enemy->GetTargetTeam()) {
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
        Vector2 tPos = enemy->GetTargetTeam()->GetActivePaladin()->GetPosition();
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
