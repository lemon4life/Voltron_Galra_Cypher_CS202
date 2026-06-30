#include "Entities/Enemy.h"
#include "Entities/Player.h"
#include "Core/GameManager.h"

Enemy::Enemy(Vector2 pos, Player* t)
    : GameObject(pos), type(EnemyType::GRUNT), health(100), maxHealth(100), speed(100.0f), damage(15), 
      target(t), currentState(nullptr), attackCooldown(0.0f), bossSkillCooldown(2.0f), burstCount(0), burstTimer(0.0f)
{
    currentState = &idleState;
    currentState->Enter(this);
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
    Color col = (type == EnemyType::BOSS) ? ORANGE : PURPLE;
    DrawRectangleRec(GetBoundingBox(), col);
    
    // Draw Health Bar
    float hpPercent = (float)health / maxHealth;
    float barWidth = (type == EnemyType::BOSS) ? 64.0f : 32.0f;
    float xOffset = (type == EnemyType::BOSS) ? 32.0f : 16.0f;
    float yOffset = (type == EnemyType::BOSS) ? 36.0f : 20.0f;
    DrawRectangle(position.x - xOffset, position.y - yOffset, barWidth * hpPercent, 4, RED);
}

void Enemy::ChangeState(IEnemyState* newState) {
    if (currentState != newState) {
        if (currentState) currentState->Exit(this);
        currentState = newState;
        if (currentState) currentState->Enter(this);
    }
}

#include "Core/AudioManager.h"

void Enemy::TakeDamage(int amount) {
    health -= amount;
    if (health < 0) health = 0;
    AudioManager::GetInstance().PlaySoundEffect("hit");
}

Rectangle Enemy::GetBoundingBox() const {
    if (type == EnemyType::BOSS) {
        // 64x64 bounding box centered on position
        return { position.x - 32.0f, position.y - 32.0f, 64.0f, 64.0f };
    }
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
