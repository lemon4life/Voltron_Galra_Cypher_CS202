#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include <cmath>
#include <iostream>

Paladin::Paladin(Vector2 pos, CharacterSprites sprites, int maxHp, float maxEx)
    : Character(pos, 150.0f, maxHp, sprites.battleIdle),
      sprites(sprites),
      teamManager(nullptr),
      currentWeapon(nullptr),
      currentFrame(0),
      frameTimer(0.0f),
      frameDuration(0.1f), // 10 fps animation speed
      facingLeft(false),
      numFrames(12),
      maxHealth(maxHp),
      ghostHp(maxHp),
      exEnergy(0.0f),
      maxExEnergy(maxEx),
      dashCooldown(0.0f),
      dashTimer(0.0f),
      isInvincible(false),
      lastMoveDir{1.0f, 0.0f}, // Initialize pointing right
      footstepTimer(0.0f)
{
    currentState = &idleState;
    currentState->Enter(this);
}

Paladin::~Paladin() {
    if (currentState) {
        currentState->Exit(this);
    }
        if (currentWeapon) {
        delete currentWeapon;
    }
}

Vector2 Paladin::GetWeaponPivot() const {
    Vector2 pivot = position;
    // Align to the back shoulder (left side when facing right, right side when facing left)
    if (facingLeft) {
        pivot.x += 12.0f;
    } else {
        pivot.x -= 12.0f;
    }
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
    
    if (teamManager) {
        int remainingDamage = teamManager->TakeArmorDamage(amount);
        if (remainingDamage > 0) {
            health -= remainingDamage;
            if (health < 0) health = 0;
            teamManager->NotifyObservers(); // Update UI with new HP
        }
    } else {
        // Fallback if no team manager
        health -= amount;
        if (health < 0) health = 0;
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
    
    if (GameManager::GetInstance().GetState() == GameState::HUB || GameManager::GetInstance().GetState() == GameState::MENU) {
        if (lastMoveDir.x < 0.0f) facingLeft = true;
        else if (lastMoveDir.x > 0.0f) facingLeft = false;
    } else {
        facingLeft = (aimTarget.x < position.x);
    }

        if (currentWeapon) {
        currentWeapon->SetAim(dir, angle);
    }

    // Decrement dash cooldown over time
    if (dashCooldown > 0.0f) {
        dashCooldown -= deltaTime;
        if (dashCooldown < 0.0f) {
            dashCooldown = 0.0f;
        }
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
}

Rectangle Paladin::GetBoundingBox() const {
    // 24x36 bounding box centered on position
    return { position.x - 12.0f, position.y - 18.0f, 24.0f, 36.0f };
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
        std::round(position.x),
        std::round(position.y),
        frameWidth,
        frameHeight
    };

    Vector2 origin = { frameWidth / 2.0f, frameHeight / 2.0f };

    Color tint = isInvincible ? GRAY : WHITE;

    DrawTexturePro(texture, sourceRec, destRec, origin, 0.0f, tint);
    
    if (currentWeapon && GameManager::GetInstance().GetState() != GameState::HUB) {
        currentWeapon->Draw(GetWeaponPivot(), facingLeft);
    }
}

Texture2D Paladin::GetIdleTexture() const {
    GameState state = GameManager::GetInstance().GetState();
    if (state == GameState::HUB || state == GameState::MENU) {
        return sprites.restIdle;
    }
    return sprites.battleIdle;
}

Texture2D Paladin::GetRunTexture() const {
    GameState state = GameManager::GetInstance().GetState();
    if (state == GameState::HUB || state == GameState::MENU) {
        return sprites.restRun;
    }
    return sprites.battleRun;
}

void Paladin::UpdateFootsteps(float dt) {
    footstepTimer += dt;
    if (footstepTimer >= 0.3f) {
        footstepTimer = 0.0f;
        AudioManager::GetInstance().PlaySequentialFootstep();
    }
}

void Paladin::OnHitEnemy(int damage) {
    exEnergy += (float)damage * 0.5f; // 50% of damage converts to EX
    if (exEnergy > maxExEnergy) {
        exEnergy = maxExEnergy;
    }
}
