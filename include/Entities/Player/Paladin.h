#pragma once
#include "Entities/Character.h"
#include "Entities/Player/PlayerState.h"
#include "Entities/Player/PlayerDashState.h"
#include "Entities/Player/PlayerAttackState.h"
#include "Entities/Player/PaladinDefinition.h"
#include "Combat/IAttackStrategy.h"
#include <vector>

struct CharacterSprites {
    Texture2D idle;
    Texture2D run;
    Texture2D weapon;
    Texture2D muzzleFlash;
    Texture2D attack1;
    Texture2D attack2;
    Texture2D bullet;
    Texture2D impact;
    Texture2D dashFront;
    Texture2D dashBack;
    Texture2D parry;
    Texture2D down;
};

class TeamManager;

class Paladin : public Character {
protected:
    IPlayerState* currentState;
    IAttackStrategy* currentWeapon;
    
    CharacterSprites sprites;
    TeamManager* teamManager;
    PaladinId paladinId;

    // Animation specific
    int currentFrame;
    float frameTimer;
    float frameDuration;
    bool facingLeft;
    int numFrames; // Default 4

    // State instances
    PlayerIdleState idleState;
    PlayerRunState runState;
    PlayerParryState parryState;
    PlayerDashState dashState;
    PlayerAttackState attackState;
    PlayerDownState downState;

    // Individual Stats
    int maxHealth;
    float ghostHp;
    float exEnergy;
    float maxExEnergy;

    // Dash mechanic properties
    float dashCooldown;
    float attackCooldown;
    float dashTimer;

    bool isInvincible;
    bool isParrying;
    bool parrySuccess;
    int consecutiveParries;
    float parryAngle;
    Vector2 lastMoveDir;
    Vector2 knockbackVelocity;
    float footstepTimer;
    float renderOffsetY;
        float swapParryWindowTimer;
    float autoParryDurationTimer;
    bool isAutoParry;
    Vector2 aimTarget;
    class Enemy* lockedEnemy;
    float currentAimAngle;
    float targetAimAngle;

public:
    Paladin(
        Vector2 pos,
        CharacterSprites sprites,
        const PaladinDefinition& definition
    );
    virtual ~Paladin();

    void Update(float deltaTime) override;
    void Draw() override;
    void SetParrying(bool parry);
    int GetConsecutiveParries() const { return consecutiveParries; }
    void IncrementParryCount() { 
        consecutiveParries++; 
        if (consecutiveParries >= 3 && isParrying) {
            ChangeState(&idleState);
        }
    }
    void ResetParryCount() { consecutiveParries = 0; }
    bool IsParrying() const { return isParrying; }
    bool CanParryAttack(Vector2 attackerPos) const;
    void TriggerParrySuccess(GameObject* attacker);
    void ApplyKnockback(Vector2 dir, float force);
    
    void SetAimTarget(Vector2 target) { if (!isParrying) aimTarget = target; }
    Vector2 GetAimTarget() const { return aimTarget; }
    
    void SetLockedEnemy(class Enemy* target) { lockedEnemy = target; }
    class Enemy* GetLockedEnemy() const { return lockedEnemy; }
    
    float GetCurrentAimAngle() const { return currentAimAngle; }
    void SetCurrentAimAngle(float angle) { currentAimAngle = angle; }
    
    float GetTargetAimAngle() const { return targetAimAngle; }
    void SetTargetAimAngle(float angle) { targetAimAngle = angle; }
    
    void SetTeamManager(TeamManager* manager) { teamManager = manager; }
    TeamManager* GetTeamManager() const { return teamManager; }

    void ChangeState(IPlayerState* newState);
    void Attack();
    
    virtual void UseSkill() = 0;
    virtual void UseUltimate() = 0;
    
    Vector2 GetWeaponPivot() const;
    void SetWeapon(IAttackStrategy* weapon) { currentWeapon = weapon; }
    IAttackStrategy* GetCurrentWeapon() const { return currentWeapon; }
    
    virtual void TakeDamage(int amount);
    void OnHitEnemy(int damage);
    void ResetStats();
    void UpdateFootsteps(float dt);

    Rectangle GetBoundingBox() const override;
    Rectangle GetCollisionBox() const override;
    bool CheckCollision(const std::vector<GameObject*>& entities) const;

    // Getters
    int GetHealth() const { return health; }
    int GetMaxHealth() const { return maxHealth; }
    PaladinId GetPaladinId() const { return paladinId; }
    float GetGhostHp() const { return ghostHp; }
    float GetExEnergy() const { return exEnergy; }
    float GetMaxExEnergy() const { return maxExEnergy; }
    
    PlayerIdleState* GetIdleState() { return &idleState; }
    PlayerRunState* GetRunState() { return &runState; }
    PlayerParryState* GetParryState() { return &parryState; }
    PlayerDashState* GetDashState() { return &dashState; }
    PlayerAttackState* GetAttackState() { return &attackState; }
    PlayerDownState* GetDownState() { return &downState; }
    
    Texture2D GetIdleTexture() const;
    Texture2D GetRunTexture() const;
    Texture2D GetDashFrontTexture() const;
    Texture2D GetDashBackTexture() const;
    Texture2D GetParryTexture() const;
    Texture2D GetDownTexture() const;

    // Animation helpers
    void UpdateAnimation(float deltaTime);
    void SetNumFrames(int frames) { numFrames = frames; }
    void ResetAnimation() { currentFrame = 0; frameTimer = 0.0f; }
    void SetFacingLeft(bool left) { facingLeft = left; }
    bool IsFacingLeft() const { return facingLeft; }
    // Returns the source rectangle for the current animation frame (used by particle effects)
    Rectangle GetCurrentSourceRect() const {
        float fw = (float)texture.width / (float)numFrames;
        float fh = (float)texture.height;
        return { (float)currentFrame * fw, 0.0f, fw, fh };
    }

    // Dash Getters and Setters
    float GetDashCooldown() const { return dashCooldown; }
    float GetAttackCooldown() const { return attackCooldown; }
    void SetAttackCooldown(float val) { attackCooldown = val; }
    void SetDashCooldown(float cooldown) { dashCooldown = cooldown; }
    float GetDashTimer() const { return dashTimer; }
    void SetDashTimer(float timer) { dashTimer = timer; }
    bool IsInvincible() const { return isInvincible; }
    void SetInvincible(bool invincible) { isInvincible = invincible; }
    Vector2 GetLastMoveDir() const { return lastMoveDir; }
    void SetLastMoveDir(Vector2 dir) { lastMoveDir = dir; }

    void TriggerSwapParryWindow() { swapParryWindowTimer = 0.2f; }
    void DecrementSwapParryWindow(float dt) { if (swapParryWindowTimer > 0.0f) swapParryWindowTimer -= dt; }
    bool IsAutoParrying() const { return isAutoParry; }
    void SetAutoParry(bool val) { isAutoParry = val; }
    float GetAutoParryDurationTimer() const { return autoParryDurationTimer; }
    void DecrementAutoParryDuration(float dt) { if (autoParryDurationTimer > 0.0f) autoParryDurationTimer -= dt; }
    void ResetAutoParryDuration() { autoParryDurationTimer = 1.0f; }

    float GetRenderOffsetY() const { return renderOffsetY; }
    void SetRenderOffsetY(float offset) { renderOffsetY = offset; }
};
