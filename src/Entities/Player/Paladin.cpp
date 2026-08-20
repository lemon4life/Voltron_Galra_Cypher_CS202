#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/InputManager.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Constants.h"
#include "Entities/Projectile.h"

#include <cmath>
#include <iostream>

Paladin::Paladin(
    Vector2 pos,
    CharacterSprites sprites,
    const PaladinDefinition& definition
)
    : Character(pos, BaseStats::Speed * definition.speedScalar, BaseStats::HP * definition.hpScalar, sprites.idle),
      currentWeapon(nullptr),
      sprites(sprites),
      teamManager(nullptr),
      paladinId(definition.id),
      currentFrame(0),
      frameTimer(0.0f),
      frameDuration(0.1f), // 10 fps animation speed
      facingLeft(false),
      numFrames(4),
      maxHealth(BaseStats::HP * definition.hpScalar),
      ghostHp(BaseStats::HP * definition.hpScalar),
      exEnergy(0.0f),
      maxExEnergy(definition.maxExEnergy),
      displayedHp(static_cast<float>(BaseStats::HP * definition.hpScalar)),
      displayedExEnergy(0.0f),
      dashCooldown(0.0f),
      attackCooldown(BaseStats::AttackCooldown * definition.attackCooldownScalar),
      dashTimer(0.0f),
      isInvincible(false),
      isParrying(false),
      parrySuccess(false),
      consecutiveParries(0),
      parryAngle(0.0f),
      lastMoveDir{1.0f, 0.0f},
      knockbackVelocity{0.0f, 0.0f}, // Initialize pointing right
      footstepTimer(0.0f),
      renderOffsetY(0.0f),
      swapParryWindowTimer(0.0f), autoParryDurationTimer(0.0f), isAutoParry(false),
      aimTarget(pos),
      lockedEnemy(nullptr),
      currentAimAngle(0.0f),
      targetAimAngle(0.0f),
      currentAimVector{1.0f, 0.0f},
      currentAimStrategy(nullptr)
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
    for (auto& buff : personalBuffs) {
        buff->OnRemove(this);
    }
    personalBuffs.clear();
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
    if (isInvincible || isInvulnerable || Constants::DEBUG_PLAYER_IMMUNITY) return;
    
    int actualDamage = std::min(health, amount);
    health -= amount;
    
    if (actualDamage > 0) {
        GameManager::GetInstance().GetEffectManager()
            .SpawnDamageNumber(position, actualDamage);
    }
    
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

void Paladin::UpdateAim(Vector2 rawMouseWorld) {
    if (Constants::isAutoAimEnabled) {
        Vector2 aimVec = GetCurrentAimVector();
        SetAimTarget({ position.x + aimVec.x * 100.0f, position.y + aimVec.y * 100.0f });
    } else {
        SetAimTarget(rawMouseWorld);
    }
}

void Paladin::TickTimers(float deltaTime) {
    if (dashCooldown > 0.0f) dashCooldown -= deltaTime;
    
    // Update personal buffs
    for (auto it = personalBuffs.begin(); it != personalBuffs.end(); ) {
        (*it)->Update(deltaTime, this);
        if ((*it)->IsFinished()) {
            (*it)->OnRemove(this);
            it = personalBuffs.erase(it);
        } else {
            ++it;
        }
    }

    // Update attached effects
    for (auto it = attachedEffects.begin(); it != attachedEffects.end();) {
        it->lifetime -= deltaTime;
        if (it->lifetime <= 0.0f) {
            it = attachedEffects.erase(it);
        } else {
            float progress = 1.0f - (it->lifetime / it->maxLifetime);
            it->currentFrame = (int)(progress * it->numFrames);
            if (it->currentFrame >= it->numFrames) it->currentFrame = it->numFrames - 1;
            ++it;
        }
    }
}

void Paladin::AddAttachedEffect(Texture2D tex, int frames, float lifetime) {
    AttachedEffect effect;
    effect.texture = tex;
    effect.numFrames = frames;
    effect.currentFrame = 0;
    effect.lifetime = lifetime;
    effect.maxLifetime = lifetime;
    attachedEffects.push_back(effect);
}

Projectile* Paladin::SpawnLinearProjectile(Vector2 dir, float speed, int damage, float maxFlyTime, bool piercing, Texture2D tex, bool fixedRotation) {
    Vector2 vel = Vector2Scale(dir, speed);
    Projectile* proj = new Projectile(GetWeaponPivot(), vel, 5.0f, damage, tex, false);
    proj->SetReturning(false);
    proj->SetPiercing(piercing);
    proj->SetMaxFlyTime(maxFlyTime);
    proj->SetOwner(this);
    
    if (fixedRotation) {
        float rot = atan2(dir.y, dir.x) * (180.0f / PI);
        proj->SetFixedRotation(true, rot);
    }
    
    GameManager::GetInstance().AddProjectile(proj);
    return proj;
}

#include "raymath.h"
void Paladin::Update(float deltaTime) {
    TickTimers(deltaTime);

    displayedHp = Lerp(displayedHp, static_cast<float>(health), 10.0f * deltaTime);
    displayedExEnergy = Lerp(displayedExEnergy, exEnergy, 10.0f * deltaTime);

    if (autoParryDurationTimer > 0.0f) {
        autoParryDurationTimer -= deltaTime;
        if (autoParryDurationTimer <= 0.0f) {
            isAutoParry = false;
        }
    }

    DecrementSwapParryWindow(deltaTime);
    // Update Ghost HP
    if (ghostHp > health) {
        ghostHp -= maxHealth * deltaTime * 0.5f; // Drain at 50% max HP per second
        if (ghostHp < health) ghostHp = health;
    } else if (ghostHp < health) {
        ghostHp = health; // Instant catch up on heal
    }

    if (currentAimStrategy) {
        currentAimVector = currentAimStrategy->CalculateAimVector(this);
        targetAimAngle = atan2f(currentAimVector.y, currentAimVector.x);
    }

    // Smooth shortest-path angular interpolation
    float diff = targetAimAngle - currentAimAngle;
    while (diff > PI) diff -= 2.0f * PI;
    while (diff < -PI) diff += 2.0f * PI;
    
    float lerpSpeed = 15.0f;
    currentAimAngle += diff * lerpSpeed * deltaTime;
    
    while (currentAimAngle > PI) currentAimAngle -= 2.0f * PI;
    while (currentAimAngle < -PI) currentAimAngle += 2.0f * PI;

    Vector2 dir = { cosf(currentAimAngle), sinf(currentAimAngle) };
    float angle = currentAimAngle * (180.0f / PI);
    
    if (GameManager::GetInstance().GetState() == GameState::HUB || GameManager::GetInstance().GetState() == GameState::MAIN_MENU) {
        if (lastMoveDir.x < 0.0f) facingLeft = true;
        else if (lastMoveDir.x > 0.0f) facingLeft = false;
    } else {
        facingLeft = (dir.x < 0.0f);
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


    // Apply knockback physics
    if (Vector2Length(knockbackVelocity) > 5.0f) {
        // Smooth exponential decay (similar to weapon recoil lerping)
        knockbackVelocity.x -= knockbackVelocity.x * 15.0f * deltaTime;
        knockbackVelocity.y -= knockbackVelocity.y * 15.0f * deltaTime;
        
        Vector2 currentPos = GetPosition();
        LevelManager* levelManager = GameManager::GetInstance().GetLevelManager();
        float levelWidth = GameManager::GetInstance().GetLevelWidth();
        float levelHeight = GameManager::GetInstance().GetLevelHeight();
        
        Vector2 prevPos = currentPos;

        // X Movement
        currentPos.x += knockbackVelocity.x * deltaTime;
        if (currentPos.x < 0.0f) currentPos.x = 0.0f;
        if (currentPos.x > levelWidth) currentPos.x = levelWidth;
        SetPosition(currentPos);
        if (levelManager && levelManager->IsSolidCollision(GetCollisionBox())) {
            currentPos.x = prevPos.x;
            SetPosition(currentPos);
            knockbackVelocity.x = 0.0f;
        }

        prevPos = GetPosition();

        // Y Movement
        currentPos.y += knockbackVelocity.y * deltaTime;
        if (currentPos.y < 0.0f) currentPos.y = 0.0f;
        if (currentPos.y > levelHeight) currentPos.y = levelHeight;
        SetPosition(currentPos);
        if (levelManager && levelManager->IsSolidCollision(GetCollisionBox())) {
            currentPos.y = prevPos.y;
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

void Paladin::UpdateInactive(float deltaTime) {
    TickTimers(deltaTime);
}

void Paladin::DrawInactive() {
    // If you want any visual effect for inactive characters (like particle trails), it could go here
}

void Paladin::ResetStats() {
    health = maxHealth;
    ghostHp = maxHealth;
    displayedHp = static_cast<float>(maxHealth);
    exEnergy = 0.0f;
    displayedExEnergy = 0.0f;
    dashCooldown = 0.0f;
    dashTimer = 0.0f;
    isInvincible = false;
    isInvulnerable = false;
    invulnerabilityTimer = 0.0f;
    isParrying = false;
    parrySuccess = false;
    consecutiveParries = 0;
    knockbackVelocity = {0.0f, 0.0f};
    swapParryWindowTimer = 0.0f;
    autoParryDurationTimer = 0.0f;
    isAutoParry = false;
    lockedEnemy = nullptr;
    currentAimAngle = 0.0f;
    targetAimAngle = 0.0f;
    currentAimVector = {1.0f, 0.0f};
    lastMoveDir = {1.0f, 0.0f};
    texture = GetIdleTexture();
    renderOffsetY = 0.0f;
    ultimateCooldownTimer = 0.0f;
    ChangeState(&idleState);
    ResetAnimation();
}

Rectangle Paladin::GetBoundingBox() const {
    // 16x24 bounding box centered on position for 32x32 sprite
    return { position.x - 8.0f, position.y - 12.0f, 16.0f, 24.0f };
}

Rectangle Paladin::GetCollisionBox() const {
    constexpr float HORIZONTAL_INSET = 1.0f;
    return {
        position.x - Constants::RENDER_TILE_SIZE * 0.5f + HORIZONTAL_INSET,
        position.y + 6.0f,
        Constants::RENDER_TILE_SIZE - HORIZONTAL_INSET * 2.0f,
        8.0f
    };
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
    for (auto& buff : personalBuffs) {
        buff->Draw(this);
    }
    
    // Auto parry visual indication
    const float frameWidth = (float)texture.width / numFrames;
    const float frameHeight = (float)texture.height;

    if (isInvulnerable) {
        Vector2 center = { position.x , position.y  };
        DrawCircleLines(center.x, center.y, 20.0f, ColorAlpha(YELLOW, 0.4f));
        DrawCircleLines(center.x, center.y, 22.0f, ColorAlpha(YELLOW, 0.2f));
    }

    // Draw Player Circle underneath
    Texture2D circleTex = AssetManager::GetInstance().GetTexture("Player_Circle");
    if (circleTex.id != 0) {
        Rectangle dest = { position.x, position.y + 17.0f, (float)circleTex.width, (float)circleTex.height };
        Vector2 circleOrigin = { (float)circleTex.width / 2.0f, (float)circleTex.height / 2.0f };
        DrawTexturePro(circleTex, {0, 0, (float)circleTex.width, (float)circleTex.height}, dest, circleOrigin, 0.0f, WHITE);
    }

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
    
    if (currentWeapon && IsWeaponVisible() && GameManager::GetInstance().GetState() != GameState::HUB && health > 0) {
        Vector2 pivot = GetWeaponPivot();
        if (isParrying) {
            Vector2 dir = { cosf(currentAimAngle), sinf(currentAimAngle) };
            
            pivot.x += dir.x * 12.0f;
            pivot.y += dir.y * 12.0f;
        }
        currentWeapon->Draw(pivot, facingLeft);
    }

    for (const auto& effect : attachedEffects) {
        if (effect.texture.id != 0) {
            float eFrameWidth = (float)effect.texture.width / effect.numFrames;
            float eFrameHeight = (float)effect.texture.height;
            Rectangle eSource = { effect.currentFrame * eFrameWidth, 0.0f, eFrameWidth, eFrameHeight };
            Rectangle eDest = { position.x, position.y + renderOffsetY, eFrameWidth, eFrameHeight };
            Vector2 eOrigin = { eFrameWidth / 2.0f, eFrameHeight / 2.0f };
            DrawTexturePro(effect.texture, eSource, eDest, eOrigin, 0.0f, WHITE);
        }
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
    exEnergy += (float)damage * 0.15f; // Slower EX generation
    if (exEnergy > maxExEnergy) {
        exEnergy = maxExEnergy;
    }
}

void Paladin::SetParrying(bool parry) {
    isParrying = parry;
    if (!parry) parrySuccess = false;
}

void Paladin::TriggerParrySuccess(GameObject* attacker) {
    if (swapParryWindowTimer > 0.0f || isAutoParry) {
        isAutoParry = true;
        ResetAutoParryDuration();
        if (currentState != &parryState) {
            ChangeState(&parryState);
        }
    }
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
    if (swapParryWindowTimer > 0.0f) return true; // Omnidirectional perfect parry window on swap
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
