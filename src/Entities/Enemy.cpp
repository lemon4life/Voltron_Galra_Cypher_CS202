#include "Entities/Enemy.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/LevelManager.h"
#include <iostream>
#include "raymath.h"

Enemy::Enemy(Vector2 pos, TeamManager* t, IEntityRemovalAccess* removalAccess)
    : GameObject(pos), health(100), maxHealth(100), speed(100.f), damage(15),
      attackCooldown(0.1f), baseAttackCooldown(0.1f), size({32.0f, 32.0f}), enemyType(EnemyType::GRUNT),
      targetTeam(t), currentState(nullptr), removalAccess(removalAccess), knockbackVelocity{0.0f, 0.0f}
{}


Enemy::Enemy(
    Vector2 pos,
    TeamManager* t,
    int imaxHealth,
    float ispeed,
    int idamage,
    float iattackCooldown,
    IEntityRemovalAccess* removalAccess
)
    : GameObject(pos), health(imaxHealth), maxHealth(imaxHealth), speed(ispeed), damage(idamage),
      attackCooldown(iattackCooldown), baseAttackCooldown(iattackCooldown), size({32.0f, 32.0f}), enemyType(EnemyType::GRUNT),
      targetTeam(t), currentState(nullptr), removalAccess(removalAccess), knockbackVelocity{0.0f, 0.0f}
{}

Enemy::~Enemy() {

}

// void Enemy::Update(float deltaTime) {
//     if (currentState) {
//         currentState->Update(this, deltaTime);
//     }
// }

// void Enemy::Draw() {
//     Color col = (enemyType== EnemyType::BOSS) ? ORANGE : PURPLE;
//     DrawRectangleRec(GetBoundingBox(), col);
    
//     // Draw Health Bar
//     float hpPercent = (float)health / maxHealth;
//     float barWidth = (enemyType == EnemyType::BOSS) ? 64.0f : 32.0f;
//     float xOffset = (enemyType == EnemyType::BOSS) ? 32.0f : 16.0f;
//     float yOffset = (enemyType == EnemyType::BOSS) ? 36.0f : 20.0f;
//     DrawRectangle(position.x - xOffset, position.y - yOffset, barWidth * hpPercent, 4, RED);
// }

void Enemy::ChangeState(IEnemyState* newState) {
    if (!newState || currentState == newState) return;

    if (currentState) {
        currentState->Exit(this);
    }

    currentState = newState;
    currentState->Enter(this);
}

void Enemy::ResetAttackCooldown() {
    attackCooldown = baseAttackCooldown;
}

void Enemy::TakeDamage(int amount) {
    if (health <= 0) return;

    health -= amount;
    if (health < 0) health = 0;
    AudioManager::GetInstance().PlaySoundEffect("hit");

    if (health <= 0 && !deathNotified) {
        deathNotified = true;
        if (removalAccess) {
            removalAccess->QueueRemoval(this);
        }
    }
}

Rectangle Enemy::GetBoundingBox() const {
    return { position.x - size.x/2.f, position.y - size.y/2.f, size.x, size.y };
}

bool Enemy::CheckCollision(const std::vector<GameObject*>& entities) const {
    Rectangle myBox = GetBoundingBox();
    for (auto* entity : entities) {
        if (entity != this && CheckCollisionRecs(myBox, entity->GetBoundingBox())) {
            return true;
        }
    }
    return false;
}

void Enemy::ApplyKnockback(Vector2 dir, float force) {
    knockbackVelocity.x += dir.x * force;
    knockbackVelocity.y += dir.y * force;
}

void Enemy::UpdateKnockback(float deltaTime) {
    if (Vector2Length(knockbackVelocity) > 5.0f) {
        knockbackVelocity.x -= knockbackVelocity.x * 15.0f * deltaTime;
        knockbackVelocity.y -= knockbackVelocity.y * 15.0f * deltaTime;
        
        Vector2 currentPos = GetPosition();
        LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
        float levelWidth = GameManager::GetInstance().GetLevelWidth();
        float levelHeight = GameManager::GetInstance().GetLevelHeight();
        
        currentPos.x += knockbackVelocity.x * deltaTime;
        if (currentPos.x < 0.0f) currentPos.x = 0.0f;
        if (currentPos.x > levelWidth) currentPos.x = levelWidth;
        SetPosition(currentPos);
        if (levelManager && levelManager->IsSolidCollision(GetBoundingBox())) {
            currentPos.x -= knockbackVelocity.x * deltaTime;
            SetPosition(currentPos);
            knockbackVelocity.x = 0.0f;
        }
        
        currentPos.y += knockbackVelocity.y * deltaTime;
        if (currentPos.y < 0.0f) currentPos.y = 0.0f;
        if (currentPos.y > levelHeight) currentPos.y = levelHeight;
        SetPosition(currentPos);
        if (levelManager && levelManager->IsSolidCollision(GetBoundingBox())) {
            currentPos.y -= knockbackVelocity.y * deltaTime;
            SetPosition(currentPos);
            knockbackVelocity.y = 0.0f;
        }
    } else {
        knockbackVelocity = {0.0f, 0.0f};
    }
}
