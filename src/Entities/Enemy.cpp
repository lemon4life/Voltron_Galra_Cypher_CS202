#include "Entities/Enemy.h"
#include "AI/EnemyCollision.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/AudioManager.h"
#include "raymath.h"

Enemy::Enemy(
    Vector2 pos,
    TeamManager* t,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess
)
    : GameObject(pos, GameObjectType::Enemy),
      health(100), maxHealth(100), speed(100.f), damage(15),
      attackCooldown(0.1f), baseAttackCooldown(0.1f), dazeDuration(2.0f),
      size({32.0f, 32.0f}),
      knockbackVelocity{0.0f, 0.0f}, enemyType(EnemyType::GRUNT),
      targetTeam(t), currentState(nullptr), removalAccess(removalAccess),
      pathAccess(pathAccess)
{
    dazeState = std::make_unique<EnemyDazeState>();
}


Enemy::Enemy(
    Vector2 pos,
    TeamManager* t,
    int imaxHealth,
    float ispeed,
    int idamage,
    float iattackCooldown,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess
)
    : GameObject(pos, GameObjectType::Enemy),
      health(imaxHealth), maxHealth(imaxHealth), speed(ispeed),
      damage(idamage), attackCooldown(iattackCooldown),
      baseAttackCooldown(iattackCooldown), dazeDuration(2.0f),
      size({32.0f, 32.0f}),
      knockbackVelocity{0.0f, 0.0f}, enemyType(EnemyType::GRUNT),
      targetTeam(t), currentState(nullptr), removalAccess(removalAccess),
      pathAccess(pathAccess)
{
    dazeState = std::make_unique<EnemyDazeState>();
}

Enemy::~Enemy() {
    EndPathFinding();
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
        removalAccess.QueueRemoval(this);
    }
}

Rectangle Enemy::GetBoundingBox() const {
    return { position.x - size.x/2.f, position.y - size.y/2.f, size.x, size.y };
}

bool Enemy::CheckCollision(const std::vector<GameObject*>& entities) const {
    return EnemyCollision::CheckAnyEnemyCollision(*this, entities);
}

void Enemy::StartPathFinding() {
    if (usePathFinding) return;

    usePathFinding = true;
    pathAccess.BeginPathFinding(*this);
}

void Enemy::EndPathFinding() {
    if (!usePathFinding) return;

    pathAccess.EndPathFinding(*this);
    usePathFinding = false;
    ClearTargetPosition();
}

void Enemy::ApplyKnockback(Vector2 dir, float force) {
    knockbackVelocity.x += dir.x * force;
    knockbackVelocity.y += dir.y * force;
}

void Enemy::UpdateKnockback(float deltaTime) {
    if (Vector2Length(knockbackVelocity) > 5.0f) {
        knockbackVelocity.x -= knockbackVelocity.x * 15.0f * deltaTime;
        knockbackVelocity.y -= knockbackVelocity.y * 15.0f * deltaTime;
        
        EnemyMoveResult moveResult = EnemyCollision::MoveAgainstWalls(
            *this,
            Vector2Scale(knockbackVelocity, deltaTime),
            pathAccess,
            EnemyWallResponse::Slide
        );

        if (moveResult.blockedX) {
            knockbackVelocity.x = 0.0f;
        }

        if (moveResult.blockedY) {
            knockbackVelocity.y = 0.0f;
        }
    } else {
        knockbackVelocity = {0.0f, 0.0f};
    }
}
