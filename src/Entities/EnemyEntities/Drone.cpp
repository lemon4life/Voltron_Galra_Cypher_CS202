#include "Entities/EnemyEntities/Drone.h"
#include "AI/DroneState.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Entities/Projectiles/DroneBullet.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr float DRONE_RENDER_WIDTH = 26.0f;
    constexpr float DRONE_RENDER_HEIGHT = 20.0f;
    constexpr float DRONE_HITBOX_SCALE = 0.8f;
    constexpr float DRONE_ATTACK_COOLDOWN = 4.0f;
    constexpr float DRONE_BULLET_LIFETIME = 4.0f;
    constexpr float DRONE_BULLET_RADIUS = 4.0f;
}

Drone::Drone(
    Vector2 position,
    TeamManager* targetTeam,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess,
    ILevelLineOfSightQuery& lineOfSightQuery
) : Enemy(position, targetTeam, removalAccess, pathAccess),
    lineOfSightQuery(lineOfSightQuery) {
    
    // Setup sprites
    sprites.idle = AssetManager::GetInstance().GetTexture("Drone");
    sprites.run = sprites.idle; // Reuse idle for moving
    sprites.down = AssetManager::GetInstance().GetTexture("Drone_down");
    
    maxHealth = 200; // default health
    health = maxHealth;
    enemyType = EnemyType::DRONE;
    damage = 15;
    baseAttackCooldown = DRONE_ATTACK_COOLDOWN;
    attackCooldown = 0.0f;
    SetLocalEnemyAvoidanceEnabled(false);
    SetRenderFootOffset({ 0.0f, 25.0f });

    movingState = std::make_unique<DroneMovingState>();
    droneIdleState = std::make_unique<DroneIdleState>();
}

Drone::~Drone() {
    if (currentState) {
        currentState->Exit(this);
        currentState = nullptr;
    }
}

void Drone::Update(float deltaTime) {
    Vector2 updateStartPosition = position;
    if (UpdateSpawnSequence(deltaTime)) {
        UpdateMovementAnimationFlag(updateStartPosition);
        return;
    }

    if (!statusComponent.HasEffect(EffectType::FREEZE)) {
        hoverTime += deltaTime;
    }

    UpdateKnockback(deltaTime);

    if (statusComponent.Update(deltaTime, this)) {
        UpdateMovementAnimationFlag(updateStartPosition);
        return;
    }

    if (health <= 0) return;

    if (!currentState) {
        ChangeState(movingState.get());
    }
    if (currentState) {
        currentState->Update(this, deltaTime);
    }

    UpdateMovementAnimationFlag(updateStartPosition);
}

void Drone::Draw() {
    if (IsDead()) {
        Enemy::Draw();
        return;
    }
    
    if (!ShouldDrawDuringSpawn()) {
        DrawSpawnEffect();
        return;
    }

    float hoverOffsetY = IsSpawnSequenceActive()
        ? 0.0f
        : std::sin(hoverTime * 5.0f) * 4.0f;
    
    Rectangle source = { 0.0f, 0.0f, (float)sprites.idle.width, (float)sprites.idle.height };
    if (facingLeft) {
        source.width = -source.width;
    }
    
    Rectangle dest = {
        position.x,
        position.y + hoverOffsetY,
        DRONE_RENDER_WIDTH,
        DRONE_RENDER_HEIGHT
    };
    Vector2 origin = {
        DRONE_RENDER_WIDTH * 0.5f,
        DRONE_RENDER_HEIGHT * 0.5f
    };
    
    Color tint = statusComponent.GetStatusTint();
    DrawTexturePro(sprites.idle, source, dest, origin, 0.0f, tint);
    DrawSpawnEffect();
}

Rectangle Drone::GetBoundingBox() const {
    float width = DRONE_RENDER_WIDTH * DRONE_HITBOX_SCALE;
    float height = DRONE_RENDER_HEIGHT * DRONE_HITBOX_SCALE;
    return {
        position.x - width * 0.5f,
        position.y - height * 0.5f,
        width,
        height
    };
}

void Drone::TickAttackCooldown(float deltaTime, float rate) {
    SetAttackCooldown(std::max(
        0.0f,
        GetAttackCooldown() - std::max(0.0f, deltaTime) *
            std::max(0.0f, rate)
    ));
}

bool Drone::Attack() {
    if (!targetTeam) return false;
    
    Paladin* target = targetTeam->GetActivePaladin();
    if (!target) return false;
    
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
        DroneBullet* bullet = new DroneBullet(
            actualSpawn,
            dir,
            400.0f,
            80.0f,
            800.0f,
            DRONE_BULLET_LIFETIME,
            DRONE_BULLET_RADIUS,
            GetDamage(),
            tex,
            true
        );
        GameManager::GetInstance().AddProjectile(bullet);
    }
    
    AudioManager::GetInstance().PlayRandomLaser();
    
    ResetAttackCooldown();
    return true;
}
