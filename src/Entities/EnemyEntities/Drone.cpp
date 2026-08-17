#include "Entities/EnemyEntities/Drone.h"
#include "AI/DroneState.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Entities/Projectiles/DroneBullet.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "raymath.h"
#include <cmath>

Drone::Drone(
    Vector2 position,
    TeamManager* targetTeam,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess,
    ILevelLineOfSightQuery& lineOfSightQuery
) : Enemy(position, targetTeam, removalAccess, pathAccess),
    lineOfSightQuery(lineOfSightQuery),
    attackCooldown(0.0f) {
    
    // Setup sprites
    sprites.idle = AssetManager::GetInstance().GetTexture("Drone");
    sprites.run = sprites.idle; // Reuse idle for moving
    sprites.down = AssetManager::GetInstance().GetTexture("Drone_down");
    
    // Set bounding box/size to 26x20
    boundingBox = { position.x - 13.0f, position.y - 10.0f, 26.0f, 20.0f };
    
    maxHealth = 400; // default health
    health = maxHealth;
    
    // Initialize AI state
    activeState = std::make_unique<DroneState>();
    activeState->Enter(this);
}

Drone::~Drone() {
    if (activeState) {
        activeState->Exit(this);
    }
}

void Drone::Update(float deltaTime) {
    Vector2 updateStartPosition = position;
    if (UpdateSpawnSequence(deltaTime)) {
        UpdateMovementAnimationFlag(updateStartPosition);
        return;
    }
    UpdateKnockback(deltaTime);
    
    if (statusComponent.Update(deltaTime, this)) {
        UpdateMovementAnimationFlag(updateStartPosition);
        return;
    }

    if (health <= 0) return;
    
    if (activeState) {
        activeState->Update(this, deltaTime);
    }
    
    // Update bounding box based on new position
    boundingBox.x = position.x - 13.0f;
    boundingBox.y = position.y - 10.0f;
    
    UpdateMovementAnimationFlag(updateStartPosition);
}

void Drone::Draw() {
    if (IsDead()) {
        Enemy::Draw();
        return;
    }
    
    float hoverOffsetY = std::sin(GetTime() * 5.0f) * 4.0f;
    
    Rectangle source = { 0.0f, 0.0f, (float)sprites.idle.width, (float)sprites.idle.height };
    if (facingLeft) {
        source.width = -source.width;
    }
    
    Rectangle dest = { position.x, position.y + hoverOffsetY, 26.0f, 20.0f };
    Vector2 origin = { 13.0f, 10.0f };
    
    Color tint = statusComponent.GetStatusTint();
    DrawTexturePro(sprites.idle, source, dest, origin, 0.0f, tint);
}

void Drone::Attack() {
    if (!targetTeam) return;
    
    Paladin* target = targetTeam->GetActivePaladin();
    if (!target) return;
    
    Vector2 targetPos = target->GetPosition();
    Vector2 baseDir = Vector2Subtract(targetPos, position);
    
    float baseAngle = atan2f(baseDir.y, baseDir.x);
    
    // Origin offset (14, 13) relative to top-left of the bounding box.
    float ox = (position.x - 13.0f) + 14.0f;
    float oy = (position.y - 10.0f) + 13.0f;
    
    // Adjust for facing direction
    if (facingLeft) {
        ox = (position.x + 13.0f) - 14.0f;
    }
    Vector2 spawnPos = { ox, oy };
    
    Texture2D tex = AssetManager::GetInstance().GetTexture("Drone_bullet");
    
    // 5-way spread: -90, -45, 0, 45, 90
    float angles[5] = { -PI/2.0f, -PI/4.0f, 0.0f, PI/4.0f, PI/2.0f };
    
    for (int i = 0; i < 5; ++i) {
        float angle = baseAngle + angles[i];
        Vector2 dir = { cosf(angle), sinf(angle) };
        Vector2 actualSpawn = { spawnPos.x + dir.x * 15.0f, spawnPos.y + dir.y * 15.0f };
        
        // start with high inertia before slowing down to a steady floating speed
        DroneBullet* bullet = new DroneBullet(actualSpawn, dir, 400.0f, 80.0f, 800.0f, 4.0f, 15, tex, true);
        GameManager::GetInstance().AddProjectile(bullet);
    }
    
    AudioManager::GetInstance().PlayRandomLaser();
    
    ResetAttackCooldown();
}
