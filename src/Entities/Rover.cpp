#include "Entities/Rover.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "Entities/Projectile.h"
#include "raymath.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/ParticleManager.h"
#include <cmath>
#include <iostream>

Rover::Rover(Vector2 startPos, Paladin* owner, TeamManager* team)
    : GameObject(startPos, GameObjectType::NPC), // Prop or other
      owner(owner), teamManager(team), health(30), maxHealth(30), fireCooldown(0.5f), fireTimer(0.0f), projectileSpeed(300.0f), aggroRange(300.0f) {
    boundingBox = { startPos.x - 8, startPos.y - 8, 16.0f, 16.0f };
}

void Rover::Update(float deltaTime) {
    if (health <= 0) return;

    // Follow active player
    Paladin* currentTarget = teamManager ? teamManager->GetActivePaladin() : nullptr;
    if (currentTarget && currentTarget->GetHealth() > 0) {
        Vector2 ownerPos = currentTarget->GetPosition();
        Vector2 toOwner = { ownerPos.x - position.x, ownerPos.y - position.y };
        float dist = std::sqrt(toOwner.x * toOwner.x + toOwner.y * toOwner.y);
        
        // Teleport failsafe (The Leash)
        float teleportRadius = 400.0f;
        if (dist > teleportRadius) {
            position = ownerPos;
            boundingBox.x = position.x - 8.0f;
            boundingBox.y = position.y - 8.0f;
            return;
        }
        
        float followRadius = 60.0f;
        Vector2 desiredVelocity = {0.0f, 0.0f};
        float speed = 150.0f; // Rover maximum speed
        
        if (dist > followRadius) {
            Vector2 dir = { toOwner.x / dist, toOwner.y / dist };
            desiredVelocity = { dir.x * speed, dir.y * speed };
        }
        
        // Smoothly interpolate current velocity towards desired velocity (Acceleration / Friction)
        float smoothingFactor = 8.0f; 
        currentVelocity.x += (desiredVelocity.x - currentVelocity.x) * smoothingFactor * deltaTime;
        currentVelocity.y += (desiredVelocity.y - currentVelocity.y) * smoothingFactor * deltaTime;
        
        LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
        
        position.x += currentVelocity.x * deltaTime;
        boundingBox.x = position.x - 8.0f;
        if (levelManager && levelManager->IsBlocked(boundingBox)) {
            position.x -= currentVelocity.x * deltaTime;
            boundingBox.x = position.x - 8.0f;
            currentVelocity.x = 0.0f; // Stop momentum on impact
        }
        
        position.y += currentVelocity.y * deltaTime;
        boundingBox.y = position.y - 8.0f;
        if (levelManager && levelManager->IsBlocked(boundingBox)) {
            position.y -= currentVelocity.y * deltaTime;
            boundingBox.y = position.y - 8.0f;
            currentVelocity.y = 0.0f; // Stop momentum on impact
        }
    }
    
    // Update Hitbox
    boundingBox.x = position.x - 8.0f;
    boundingBox.y = position.y - 8.0f;

    // Cooldown
    if (fireTimer > 0.0f) {
        fireTimer -= deltaTime;
    }

    // Combat AI
    if (fireTimer <= 0.0f && teamManager) {
        Enemy* closest = nullptr;
        float minDist = aggroRange;
        
        const auto& entities = GameManager::GetInstance().GetLevelEntities();
        LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
        
        for (const auto& e : entities) {
            if (e->GetObjectType() != GameObjectType::Enemy) continue;
            Enemy* enemyPtr = static_cast<Enemy*>(e);
            if (!enemyPtr || enemyPtr->IsDead() ||
                !enemyPtr->IsEnabled()) continue;
            
            Vector2 ePos = enemyPtr->GetPosition();
            float d = Vector2Distance(position, ePos);
            if (d < minDist) {
                // Perform line-of-sight check if possible
                bool hasLOS = true;
                if (levelManager) {
                    hasLOS = levelManager->HasClearLineOfSight(position, ePos, 5.0f);
                }
                
                if (hasLOS) {
                    minDist = d;
                    closest = enemyPtr;
                }
            }
        }
        
        if (closest) {
            Vector2 ePos = closest->GetPosition();
            Vector2 dir = Vector2Normalize(Vector2Subtract(ePos, position));
            Vector2 vel = Vector2Scale(dir, projectileSpeed);
            
            Texture2D projTex = AssetManager::GetInstance().GetTexture("Lance_Bullet"); // reuse bullet, tint green
            Projectile* p = new Projectile(position, vel, 1.5f, 5, projTex, false);
            p->SetTint(GREEN);
            
            GameManager::GetInstance().AddProjectile(p);
            fireTimer = fireCooldown;
        }
    }
}

void Rover::Draw() {
    if (IsDead()) return;
    
    // Draw body
    DrawRectangle(position.x - 8, position.y - 8, 16, 16, LIGHTGRAY);
    DrawRectangleLines(position.x - 8, position.y - 8, 16, 16, DARKGRAY);
    
    // Draw mini HP bar
    float hpPercent = (float)health / maxHealth;
    DrawRectangle(position.x - 10, position.y - 15, 20, 3, RED);
    DrawRectangle(position.x - 10, position.y - 15, 20 * hpPercent, 3, GREEN);
}

void Rover::TakeDamage(int damage) {
    if (IsDead()) return;
    
    int actualDamage = std::min(health, damage);
    health -= damage;
    if (health < 0) health = 0;
    
    if (actualDamage > 0) {
        ParticleManager::GetInstance().SpawnDamageNumber(position, actualDamage);
    }
}
