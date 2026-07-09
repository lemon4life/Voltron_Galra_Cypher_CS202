#include "Entities/Enemy.h"
#include "Entities/Player/Player.h"
#include "Core/Manager/GameManager.h"
#include <algorithm>
#include <iostream>

Enemy::Enemy(Vector2 pos, Player* t)
    : GameObject(pos), health(100), maxHealth(100), speed(100.f), damage(15), target(t), currentState(nullptr), attackCooldown(0.1f)
{}


Enemy::Enemy(Vector2 pos, Player* t, int imaxHealth, float ispeed, int idamage, float iattackCooldown)
    : GameObject(pos), health(imaxHealth), maxHealth(imaxHealth), speed(ispeed), damage(idamage), target(t), currentState(nullptr), attackCooldown(iattackCooldown)
{}

Enemy::~Enemy() {

}

// void Enemy::Update(float deltaTime) {
//     if (currentState) {
//         currentState->Update(this, deltaTime);
//     }
// }

// void Enemy::Draw() {
//     DrawRectangleRec(GetBoundingBox(), PURPLE);
    
//     // Draw Health Bar
//     float hpPercent = (float)health / maxHealth;
//     DrawRectangle(position.x - 16, position.y - 20, 32 * hpPercent, 4, RED);
// }

void Enemy::ToIdleState() {
    if (currentState == idleState.get()) return;

    currentState->Exit(this);
    currentState = idleState.get();
    currentState->Enter(this);
}

void Enemy::ToChaseState() {
    if (currentState == chaseState.get()) return;

    currentState->Exit(this);
    currentState = chaseState.get();
    currentState->Enter(this);
}

#include "Core/Manager/AudioManager.h"

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
    // Bounding box centered on position and size
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
