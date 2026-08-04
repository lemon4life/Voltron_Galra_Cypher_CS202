#include "Entities/Enemy.h"
#include "AI/EnemyCollision.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Constants.h"
#include "Entities/Player/Paladin.h"
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

Rectangle Enemy::GetCollisionBox() const {
    return GetNavigationFootprintAt(position);
}

Rectangle Enemy::GetNavigationFootprintAt(Vector2 entityPosition) const {
    return {
        entityPosition.x + collisionProfile.navigationCenterOffset.x -
            collisionProfile.navigationSize.x / 2.0f,
        entityPosition.y + collisionProfile.navigationCenterOffset.y -
            collisionProfile.navigationSize.y / 2.0f,
        collisionProfile.navigationSize.x,
        collisionProfile.navigationSize.y
    };
}

Rectangle Enemy::GetContactAttackBoxAt(Vector2 entityPosition) const {
    return {
        entityPosition.x - size.x / 2.0f,
        entityPosition.y - size.y / 2.0f,
        size.x,
        size.y
    };
}

bool Enemy::IsValidPathGoalPosition(
    Vector2 candidatePosition,
    const Paladin& target
) const {
    return CheckCollisionRecs(
        GetContactAttackBoxAt(candidatePosition),
        target.GetCollisionBox()
    );
}

void Enemy::DrawPathDebug() const {
    if (!Constants::DEBUG_DRAW_ENEMY_COLLISION_BOXES) return;

    DrawRectangleLinesEx(GetBoundingBox(), 1.0f, RED);
    DrawRectangleLinesEx(GetCollisionBox(), 1.0f, SKYBLUE);
    DrawRectangleLinesEx(GetContactAttackBoxAt(position), 1.0f, YELLOW);

    for (const EnemyPathDebugPoint& point : pathDebugPoints) {
        DrawCircleV(point.position, 2.5f, point.valid ? GREEN : RED);
    }

    for (Vector2 waypoint : targetPositions) {
        DrawCircleV(waypoint, 2.0f, BLUE);
    }

    if (hasSelectedPathGoal) {
        DrawCircleLines(
            (int)selectedPathGoal.x,
            (int)selectedPathGoal.y,
            5.0f,
            LIME
        );
    }

    if (pathStatus == EnemyPathStatus::Unreachable ||
        pathStatus == EnemyPathStatus::SearchLimitReached) {
        Color markerColor = pathStatus == EnemyPathStatus::SearchLimitReached
            ? MAGENTA
            : RED;
        DrawLine(
            (int)position.x - 5,
            (int)position.y - 5,
            (int)position.x + 5,
            (int)position.y + 5,
            markerColor
        );
        DrawLine(
            (int)position.x + 5,
            (int)position.y - 5,
            (int)position.x - 5,
            (int)position.y + 5,
            markerColor
        );
    }
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


EnemyMoveResult Enemy::UpdateMovement(Vector2 desiredVelocity, float deltaTime, EnemyWallResponse response) {
    float friction = 6.0f;
    currentVelocity.x += (desiredVelocity.x - currentVelocity.x) * friction * deltaTime;
    currentVelocity.y += (desiredVelocity.y - currentVelocity.y) * friction * deltaTime;
    
    Vector2 displacement = { currentVelocity.x * deltaTime, currentVelocity.y * deltaTime };
    return EnemyCollision::MoveAgainstWalls(*this, displacement, pathAccess, response);
}
