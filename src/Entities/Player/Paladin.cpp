#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/AssetManager.h"

#include <cmath>
#include <iostream>

Paladin::Paladin(Vector2 pos, CharacterSprites sprites, int maxHp, float maxEx)
    : Character(pos, 150.0f, maxHp, sprites.idle),
      sprites(sprites),
      teamManager(nullptr),
      currentWeapon(nullptr),
      currentFrame(0),
      frameTimer(0.0f),
      frameDuration(0.1f), // 10 fps animation speed
      facingLeft(false),
      numFrames(4),
      maxHealth(maxHp),
      ghostHp(maxHp),
      exEnergy(0.0f),
      maxExEnergy(maxEx),
      dashCooldown(0.0f),
      attackCooldown(0.2f),
      dashTimer(0.0f),
      isInvincible(false),
      isParrying(false),
      parrySuccess(false),
      consecutiveParries(0),
      parryAngle(0.0f),
      lastMoveDir{1.0f, 0.0f},
      knockbackVelocity{0.0f, 0.0f}, // Initialize pointing right
      footstepTimer(0.0f),
      renderOffsetY(0.0f)
{
    currentState = &idleState;
    currentState->Enter(this);
}

Paladin::~Paladin() {
    if (IsKeyPressed(KEY_F) && dashCooldown <= 0.0f && !isParrying) {
        ChangeState(&parryState);
    }
    
    if (currentState) {
        currentState->Exit(this);
    }
        if (currentWeapon) {
        delete currentWeapon;
    }
}

Vector2 Paladin::GetWeaponPivot() const {
    Vector2 pivot = position; // Position is the center of the 32x32 sprite
    
    // Top-left is (position.x - 16, position.y - 16)
    // Desired right-facing X is top-left.x + 12 -> position.x - 4
    // Desired mirrored left-facing X is top-right.x - 12 -> position.x + 4
    if (facingLeft) {
        pivot.x += 4.0f;
    } else {
        pivot.x -= 4.0f;
    }
    
    // Desired Y is top-left.y + 22 -> position.y + 6
    pivot.y += 6.0f;
    
    return pivot;
}

void Paladin::Attack() {
        if (currentWeapon) {
        currentWeapon->Attack(GetWeaponPivot());
    }
}

void Paladin::ChangeState(IPlayerState* newState) {
    if (currentState != newState) {
        if (currentState) currentState->Exit(this);
        currentState = newState;
        if (currentState) currentState->Enter(this);
    }
}

void Paladin::TakeDamage(int amount) {
    if (isInvincible) return; // Ignore damage if invincible
    
    exEnergy = 0.0f; // Clear EX on damage
    
    health -= amount;
    
    if (health <= 0) {
        health = 0;
        if (currentState != &downState) {
            ChangeState(&downState);
        }
    }
    
    if (teamManager) {
        teamManager->NotifyObservers(); // Update UI with new HP
    }
}

void Paladin::Update(float deltaTime) {
    // Update Ghost HP
    if (ghostHp > health) {
        ghostHp -= maxHealth * deltaTime * 0.5f; // Drain at 50% max HP per second
        if (ghostHp < health) ghostHp = health;
    } else if (ghostHp < health) {
        ghostHp = health; // Instant catch up on heal
    }

    Vector2 dir = Vector2Subtract(aimTarget, position);
    float distance = Vector2Length(dir);
    if (distance > 0.0f) {
        dir = Vector2Normalize(dir);
    } else {
        dir = {1.0f, 0.0f};
    }
    float angle = atan2f(dir.y, dir.x) * (180.0f / PI);
    
    if (!isParrying) {
        if (GameManager::GetInstance().GetState() == GameState::HUB || GameManager::GetInstance().GetState() == GameState::MENU) {
            if (lastMoveDir.x < 0.0f) facingLeft = true;
            else if (lastMoveDir.x > 0.0f) facingLeft = false;
        } else {
            facingLeft = (aimTarget.x < position.x);
        }
    }

    if (currentWeapon) {
        if (isParrying) {
            currentWeapon->SetAim(dir, angle - 90.0f); // 90-degree orthogonal block angle
        } else {
            currentWeapon->SetAim(dir, angle);
        }
    }

    // Decrement dash cooldown over time
    if (dashCooldown > 0.0f) {
        dashCooldown -= deltaTime;
        if (dashCooldown < 0.0f) {
            dashCooldown = 0.0f;
        }
    }

    if (IsKeyPressed(KEY_F) && dashCooldown <= 0.0f && !isParrying) {
        ChangeState(&parryState);
    }
    
    // Apply knockback physics
    if (Vector2Length(knockbackVelocity) > 5.0f) {
        // Smooth exponential decay (similar to weapon recoil lerping)
        knockbackVelocity.x -= knockbackVelocity.x * 15.0f * deltaTime;
        knockbackVelocity.y -= knockbackVelocity.y * 15.0f * deltaTime;
        
        Vector2 currentPos = GetPosition();
        LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
        float levelWidth = GameManager::GetInstance().GetLevelWidth();
        float levelHeight = GameManager::GetInstance().GetLevelHeight();
        
        // X Movement
        currentPos.x += knockbackVelocity.x * deltaTime;
        if (currentPos.x < 0.0f) currentPos.x = 0.0f;
        if (currentPos.x > levelWidth) currentPos.x = levelWidth;
        SetPosition(currentPos);
        if (levelManager && levelManager->IsSolidCollision(GetBoundingBox())) {
            currentPos.x -= knockbackVelocity.x * deltaTime;
            SetPosition(currentPos);
            knockbackVelocity.x = 0.0f;
        }
        
        // Y Movement
        currentPos.y += knockbackVelocity.y * deltaTime;
        if (currentPos.y < 0.0f) currentPos.y = 0.0f;
        if (currentPos.y > levelHeight) currentPos.y = levelHeight;
        SetPosition(currentPos);
        if (levelManager && levelManager->IsSolidCollision(GetBoundingBox())) {
            currentPos.y -= knockbackVelocity.y * deltaTime;
            SetPosition(currentPos);
            knockbackVelocity.y = 0.0f;
        }
    } else {
        knockbackVelocity = {0.0f, 0.0f};
    }

    if (currentState) {
        currentState->Update(this, deltaTime);
    }
        if (currentWeapon) {
        currentWeapon->Update(deltaTime);
    }
}

void Paladin::ResetStats() {
    health = maxHealth;
    ghostHp = maxHealth;
    exEnergy = 0.0f;
    dashCooldown = 0.0f;
    texture = GetIdleTexture();
    renderOffsetY = 0.0f;
    ChangeState(&idleState);
}

Rectangle Paladin::GetBoundingBox() const {
    // 16x24 bounding box centered on position for 32x32 sprite
    return { position.x - 8.0f, position.y - 12.0f, 16.0f, 24.0f };
}

bool Paladin::CheckCollision(const std::vector<GameObject*>& entities) const {
    Rectangle pBox = GetBoundingBox();
    for (auto* entity : entities) {
        if (CheckCollisionRecs(pBox, entity->GetBoundingBox())) {
            return true;
        }
    }
    return false;
}

void Paladin::UpdateAnimation(float deltaTime) {
    frameTimer += deltaTime;
    if (frameTimer >= frameDuration) {
        frameTimer -= frameDuration;
        currentFrame = (currentFrame + 1) % numFrames;
    }
}

void Paladin::Draw() {
    // Draw Player Circle underneath
    Texture2D circleTex = AssetManager::GetInstance().GetTexture("Player_Circle");
    if (circleTex.id != 0) {
        Rectangle dest = { position.x, position.y + 17.0f, (float)circleTex.width, (float)circleTex.height };
        Vector2 circleOrigin = { (float)circleTex.width / 2.0f, (float)circleTex.height / 2.0f };
        DrawTexturePro(circleTex, {0, 0, (float)circleTex.width, (float)circleTex.height}, dest, circleOrigin, 0.0f, WHITE);
    }

    const float frameWidth = (float)texture.width / numFrames;
    const float frameHeight = (float)texture.height;

    float sourceX = (float)currentFrame * frameWidth;
    if (facingLeft) {
        sourceX += frameWidth;
    }

    Rectangle sourceRec = {
        sourceX,
        0.0f,
        facingLeft ? -frameWidth : frameWidth,
        frameHeight
    };

    Rectangle destRec = {
        position.x,
        position.y + renderOffsetY,
        frameWidth,
        frameHeight
    };

    Vector2 origin = { frameWidth / 2.0f, frameHeight / 2.0f };

    Color tint = WHITE; // No grayscale tint during dash

    DrawTexturePro(texture, sourceRec, destRec, origin, 0.0f, tint);
    
    if (currentWeapon && GameManager::GetInstance().GetState() != GameState::HUB && health > 0) {
        Vector2 pivot = GetWeaponPivot();
        if (isParrying) {
            Vector2 dir = Vector2Subtract(aimTarget, position);
            if (Vector2Length(dir) > 0.0f) dir = Vector2Normalize(dir);
            else dir = {1.0f, 0.0f};
            
            pivot.x += dir.x * 12.0f;
            pivot.y += dir.y * 12.0f;
        }
        currentWeapon->Draw(pivot, facingLeft);
    }
}

Texture2D Paladin::GetIdleTexture() const {
    return sprites.idle;
}

Texture2D Paladin::GetRunTexture() const {
    return sprites.run;
}
Texture2D Paladin::GetDashFrontTexture() const {
    return sprites.dashFront;
}

Texture2D Paladin::GetDashBackTexture() const {
    return sprites.dashBack;
}


void Paladin::UpdateFootsteps(float dt) {
    footstepTimer += dt;
    if (footstepTimer >= 0.6f) {
        footstepTimer = 0.0f;
        AudioManager::GetInstance().PlaySequentialFootstep();
        
        // Spawn Run Dust
        Texture2D dustTex = AssetManager::GetInstance().GetTexture("Run_Dust");
        if (dustTex.id != 0) {
            Vector2 spawnPos = { position.x, position.y + 12.0f };
            GameManager::GetInstance().AddEffect(spawnPos, dustTex, 4, 0.4f, true); // drawBehind = true
        }
    }
}

void Paladin::OnHitEnemy(int damage) {
    exEnergy += (float)damage * 0.5f; // 50% of damage converts to EX
    if (exEnergy > maxExEnergy) {
        exEnergy = maxExEnergy;
    }
}

void Paladin::SetParrying(bool parry) {
    isParrying = parry;
    if (!parry) parrySuccess = false;
}

void Paladin::TriggerParrySuccess(GameObject* attacker) {
    parrySuccess = true;
    Vector2 pPos = GetPosition();
    Vector2 aPos = attacker->GetPosition();
    float angleToAttacker = atan2f(aPos.y - pPos.y, aPos.x - pPos.x) * (180.0f / PI);
    parryAngle = angleToAttacker + 90.0f; // Orthogonal
    
    // Snap aimTarget to point towards the attacker so the shield blocks that way
    Vector2 dirToAttacker = Vector2Subtract(aPos, pPos);
    if (Vector2Length(dirToAttacker) > 0.0f) {
        dirToAttacker = Vector2Normalize(dirToAttacker);
        aimTarget = { pPos.x + dirToAttacker.x * 100.0f, pPos.y + dirToAttacker.y * 100.0f };
    }
    
    // Apply knockback
    Vector2 dir = Vector2Subtract(pPos, aPos);
    if (Vector2Length(dir) > 0.0f) {
        dir = Vector2Normalize(dir);
    } else {
        dir = {1.0f, 0.0f};
    }
    ApplyKnockback(dir, 800.0f); // Fast initial impulse
}

void Paladin::ApplyKnockback(Vector2 dir, float force) {
    knockbackVelocity.x += dir.x * force;
    knockbackVelocity.y += dir.y * force;
}

Texture2D Paladin::GetParryTexture() const {
    return sprites.parry;
}

bool Paladin::CanParryAttack(Vector2 attackerPos) const {
    if (!isParrying) return false;
    if (consecutiveParries >= 3) return false;
    
    Vector2 dirToAttacker = Vector2Subtract(attackerPos, position);
    if (Vector2Length(dirToAttacker) == 0.0f) return true;
    dirToAttacker = Vector2Normalize(dirToAttacker);
    
    Vector2 aimDir = Vector2Subtract(aimTarget, position);
    if (Vector2Length(aimDir) > 0.0f) {
        aimDir = Vector2Normalize(aimDir);
    } else {
        aimDir = { facingLeft ? -1.0f : 1.0f, 0.0f };
    }
    
    float dot = (aimDir.x * dirToAttacker.x) + (aimDir.y * dirToAttacker.y);
    return dot > 0.0f; // 180-degree frontal block cone
}

Texture2D Paladin::GetDownTexture() const { return sprites.down; }
