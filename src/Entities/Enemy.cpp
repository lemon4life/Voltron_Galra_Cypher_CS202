#include "Entities/Enemy.h"
#include "Entities/Player.h"
#include "Core/GameManager.h"
#include <algorithm>
#include <iostream>

Enemy::Enemy(Vector2 pos, Player* t)
    : GameObject(pos), health(100), maxHealth(100), speed(100.f), damage(15), target(t), currentState(nullptr), attackCooldown(0.1f)
{

}


Enemy::Enemy(Vector2 pos, Player* t, int imaxHealth, float ispeed, int idamage, float iattackCooldown)
    : GameObject(pos), health(imaxHealth), maxHealth(imaxHealth), speed(ispeed), damage(idamage), target(t), currentState(nullptr), attackCooldown(iattackCooldown)
{

}

Enemy::~Enemy() {
    if (currentState) {
        currentState->Exit(this);
    }
}

void Enemy::Update(float deltaTime) {
    if (currentState) {
        currentState->Update(this, deltaTime);
    }
}

void Enemy::Draw() {
    DrawRectangleRec(GetBoundingBox(), PURPLE);
    
    // Draw Health Bar
    float hpPercent = (float)health / maxHealth;
    DrawRectangle(position.x - 16, position.y - 20, 32 * hpPercent, 4, RED);
}

void Enemy::ChangeState(IEnemyState* newState) {
    if (currentState != newState) {
        if (currentState) currentState->Exit(this);
        currentState = newState;
        if (currentState) currentState->Enter(this);
    }
}

#include "Core/AudioManager.h"

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

void Enemy::NotifyEnemyPathFind() {
    for (IEnemyObserver* observer : observers) {
        observer->OnEnemyPathFind(this);
    }
}

void Enemy::NotifyEnemyPathFindEnded() {
    for (IEnemyObserver* observer : observers) {
        observer->OnEnemyPathFindEnded(this);
    }
}

void Enemy::NotifyEnemyDied() {
    for (IEnemyObserver* observer : observers) {
        observer->OnEnemyDied(this);
    }
}

void Enemy::TakeDamage(int amount) {
    if (health <= 0) return;

    std::cout << "Take Dam: " << amount << std::endl;

    health -= amount;
    if (health < 0) health = 0;
    AudioManager::GetInstance().PlaySoundEffect("hit");

    if (health <= 0 && !deathNotified) {
        deathNotified = true;
        NotifyEnemyDied();
    }
}

Rectangle Enemy::GetBoundingBox() const {
    // 32x32 bounding box centered on position
    return { position.x - 16.0f, position.y - 16.0f, 32.0f, 32.0f };
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
