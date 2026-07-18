#include "Entities/Enemy.h"
#include "Entities/Player/Player.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include <algorithm>
#include <iostream>

Enemy::Enemy(Vector2 pos, Player* t)
    : GameObject(pos), health(100), maxHealth(100), speed(100.f), damage(15),
      attackCooldown(0.1f), baseAttackCooldown(0.1f), size({32.0f, 32.0f}), enemyType(EnemyType::GRUNT),
      target(t), currentState(nullptr)
{}


Enemy::Enemy(Vector2 pos, Player* t, int imaxHealth, float ispeed, int idamage, float iattackCooldown)
    : GameObject(pos), health(imaxHealth), maxHealth(imaxHealth), speed(ispeed), damage(idamage),
      attackCooldown(iattackCooldown), baseAttackCooldown(iattackCooldown), size({32.0f, 32.0f}), enemyType(EnemyType::GRUNT),
      target(t), currentState(nullptr)
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

void Enemy::AddObserver(IEnemyObserver* observer) {
    if (!observer) return;

    if (std::find(observers.begin(), observers.end(), observer) == observers.end()) {
        observers.push_back(observer);
    }
}

void Enemy::RemoveObserver(IEnemyObserver* observer) {
    observers.erase(
        std::remove(observers.begin(), observers.end(), observer),
        observers.end()
    );
}



void Enemy::NotifyEnemyDied() {
    for (IEnemyObserver* observer : observers) {
        observer->OnEnemyDied(this);
    }
}

void Enemy::TakeDamage(int amount) {
    if (health <= 0) return;

    health -= amount;
    if (health < 0) health = 0;
    AudioManager::GetInstance().PlaySoundEffect("hit");

    if (health <= 0 && !deathNotified) {
        deathNotified = true;
        NotifyEnemyDied();
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
