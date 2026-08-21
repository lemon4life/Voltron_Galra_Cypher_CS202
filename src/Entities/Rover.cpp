#include "Entities/Rover.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "Entities/Projectile.h"
#include "raymath.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/LevelManager.h"
#include <cmath>
#include <iostream>

Rover::Rover(Vector2 startPos, Paladin* owner, TeamManager* team)
    : GameObject(startPos, GameObjectType::NPC),
      owner(owner), teamManager(team), health(150), maxHealth(150), fireCooldown(0.5f), fireTimer(0.0f), projectileSpeed(300.0f), aggroRange(300.0f),
      isLanding(true), targetOffset({30.0f, -30.0f}), hoverTimer(0.0f), attackCooldown(1.5f), isHealing(false), healFrame(0), healFrameTimer(0.0f) {
    if (owner) {
        position.x = owner->GetPosition().x;
        position.y = owner->GetPosition().y - 600.0f;
    }
    boundingBox = { position.x - 8, position.y - 8, 16.0f, 16.0f };
    sprite = AssetManager::GetInstance().GetTexture("Rover");
}

void Rover::Heal() {
    health = maxHealth;
    isHealing = true;
    healFrame = 0;
    healFrameTimer = 0.0f;
}

void Rover::Update(float deltaTime) {
    if (health <= 0 && !isFlyingOut) {
        isFlyingOut = true;
    }

    if (isFlyingOut) {
        position.y -= 500.0f * deltaTime;
        if (owner && position.y < owner->GetPosition().y - 800.0f) {
            isRemoved = true;
        } else if (!owner && position.y < -800.0f) {
            isRemoved = true;
        }
        boundingBox.x = position.x - 8.0f;
        boundingBox.y = position.y - 8.0f;
        return;
    }

    hoverTimer += deltaTime;

    if (isHealing) {
        healFrameTimer += deltaTime;
        if (healFrameTimer >= 0.05f) {
            healFrameTimer = 0.0f;
            healFrame++;
            if (healFrame >= 8) {
                isHealing = false;
                healFrame = 0;
            }
        }
    }

    if (attackCooldown > 0.0f) {
        attackCooldown -= deltaTime;
    }

    Paladin* currentTarget = teamManager ? teamManager->GetActivePaladin() : nullptr;
    if (currentTarget && currentTarget->GetHealth() > 0) {
        Vector2 targetPos = { currentTarget->GetPosition().x + targetOffset.x, currentTarget->GetPosition().y + targetOffset.y };

        if (isLanding) {
            float dist = Vector2Distance(position, targetPos);
            if (dist < 10.0f) {
                isLanding = false;
            }
            position.x += (targetPos.x - position.x) * 5.0f * deltaTime;
            position.y += (targetPos.y - position.y) * 5.0f * deltaTime;
        } else {
            Vector2 toTarget = { targetPos.x - position.x, targetPos.y - position.y };
            float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
            
            LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
            
            bool hasLOS = true;
            if (levelManager) {
                hasLOS = levelManager->HasClearLineOfSight(position, targetPos, 5.0f);
            }
            
            float teleportRadius = hasLOS ? 400.0f : 150.0f;
            if (dist > teleportRadius) {
                position.x = owner->GetPosition().x;
                position.y = owner->GetPosition().y - 600.0f;
                isLanding = true;
                boundingBox.x = position.x - 8.0f;
                boundingBox.y = position.y - 8.0f;
                return;
            }
            
            float followRadius = 10.0f;
            Vector2 desiredVelocity = {0.0f, 0.0f};
            float speed = 250.0f;
            
            if (dist > followRadius) {
                Vector2 dir = { toTarget.x / dist, toTarget.y / dist };
                desiredVelocity = { dir.x * speed, dir.y * speed };
            }
            
            float smoothingFactor = 8.0f; 
            currentVelocity.x += (desiredVelocity.x - currentVelocity.x) * smoothingFactor * deltaTime;
            currentVelocity.y += (desiredVelocity.y - currentVelocity.y) * smoothingFactor * deltaTime;
            
            Vector2 desiredDisplacement = Vector2Scale(
                currentVelocity,
                deltaTime
            );
            Vector2 appliedDisplacement = desiredDisplacement;
            if (levelManager) {
                CollisionMovementResult movement =
                    levelManager->ResolveSolidMovement(
                        boundingBox,
                        desiredDisplacement
                    );
                appliedDisplacement = movement.appliedDisplacement;
                if (movement.blockedX) currentVelocity.x = 0.0f;
                if (movement.blockedY) currentVelocity.y = 0.0f;
            }
            position = Vector2Add(position, appliedDisplacement);
        }
        
        if (currentVelocity.x < -0.1f) facingLeft = true;
        else if (currentVelocity.x > 0.1f) facingLeft = false;
    }
    
    boundingBox.x = position.x - 8.0f;
    boundingBox.y = position.y - 8.0f;

    // Combat AI
    if (!isLanding && attackCooldown <= 0.0f && teamManager) {
        Enemy* closest = nullptr;
        float minDist = aggroRange;
        
        const auto& enemies = GameManager::GetInstance()
            .GetObjectManager().GetEnemies();
        LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
        
        for (Enemy* enemyPtr : enemies) {
            if (!enemyPtr || enemyPtr->IsDead() || !enemyPtr->IsEnabled()) continue;
            
            Vector2 ePos = enemyPtr->GetPosition();
            float d = Vector2Distance(position, ePos);
            if (d < minDist) {
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
            if (dir.x < 0) facingLeft = true;
            else if (dir.x > 0) facingLeft = false;
            
            Vector2 vel = Vector2Scale(dir, projectileSpeed);
            
            Texture2D projTex = AssetManager::GetInstance().GetTexture("Rover_bullet");
            Projectile* p = new Projectile(position, vel, 1.5f, 30, projTex, false);
            
            GameManager::GetInstance().AddProjectile(p);
            attackCooldown = 1.5f;
        }
    }
}

void Rover::Draw() {
    if (IsDead()) return;
    
    // Draw shadow
    Texture2D shadowTex = AssetManager::GetInstance().GetTexture("Player_Circle");
    if (shadowTex.id != 0) {
        Rectangle source = { 0.0f, 0.0f, (float)shadowTex.width, (float)shadowTex.height };
        Rectangle dest = { position.x, position.y + 20.0f, (float)shadowTex.width, (float)shadowTex.height };
        Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
        DrawTexturePro(shadowTex, source, dest, origin, 0.0f, {255, 255, 255, 150});
    }

    // Hover offset
    float offsetY = std::sin(hoverTimer * 5.0f) * 4.0f;

    // Draw Rover Body
    if (sprite.id != 0) {
        Rectangle source = { 0.0f, 0.0f, (float)sprite.width, (float)sprite.height };
        if (facingLeft) source.width = -source.width;
        Rectangle dest = { position.x, position.y + offsetY, (float)sprite.width, (float)sprite.height };
        Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
        DrawTexturePro(sprite, source, dest, origin, 0.0f, WHITE);
    } else {
        DrawRectangle(position.x - 8, position.y - 8 + offsetY, 16, 16, LIGHTGRAY);
        DrawRectangleLines(position.x - 8, position.y - 8 + offsetY, 16, 16, DARKGRAY);
    }
    
    // Draw HP effect
    if (isHealing) {
        Texture2D healTex = AssetManager::GetInstance().GetTexture("HP_effect");
        if (healTex.id != 0) {
            float frameWidth = healTex.width / 8.0f;
            Rectangle source = { healFrame * frameWidth, 0.0f, frameWidth, (float)healTex.height };
            Rectangle dest = { position.x, position.y + offsetY, frameWidth, (float)healTex.height };
            Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
            DrawTexturePro(healTex, source, dest, origin, 0.0f, WHITE);
        }
    }

    // Draw mini HP bar
    float barWidth = 20.0f;
    float barHeight = 3.0f;
    float fillWidth = barWidth * ((float)health / maxHealth);
    
    float barX = position.x - 10.0f;
    float barY = position.y - 20.0f; // static, no offsetY

    DrawRectangle(barX - 1.0f, barY - 1.0f, barWidth + 2.0f, barHeight + 2.0f, BLACK);
    DrawRectangle(barX, barY, barWidth, barHeight, RED);
    DrawRectangle(barX, barY, fillWidth, barHeight, GREEN);
}

void Rover::TakeDamage(int damage) {
    if (IsDead()) return;
    
    int actualDamage = std::min(health, damage);
    health -= damage;
    if (health < 0) health = 0;
    
    if (actualDamage > 0) {
        GameManager::GetInstance().GetEffectManager()
            .SpawnDamageNumber(position, actualDamage);
    }
}
