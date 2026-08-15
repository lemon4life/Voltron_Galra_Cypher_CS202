#pragma once
#include "Entities/Character.h"
#include "Entities/Player/PlayerState.h"
#include "Entities/Player/PlayerDashState.h"
#include "Entities/Player/PlayerAttackState.h"
#include "Entities/Player/PaladinDefinition.h"
#include "Combat/IAttackStrategy.h"
#include "Core/AimStrategy/IAimStrategy.h"
#include <vector>
#include <string>
#include <memory>
#include "Combat/IBuff.h"

struct BaseStats {
    static constexpr int HP = 100;
    static constexpr float Speed = 200.0f;
    static constexpr int Damage = 20;
    static constexpr float AttackCooldown = 0.5f;
};

struct UltimateIntroData {
    std::string paladinName;
    std::string ultimateName;
    Color themeColor;
    std::string portraitTextureID;
    std::string voicelineAudioID;
};

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
    
    UltimateIntroData introData;

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

    // Ultimate cooldown (separate from EX — gated by Quintessence)
    float ultimateCooldownTimer = 0.0f;
    
    // Aegis Shield Mechanic
    bool isInvulnerable = false;
    float invulnerabilityTimer = 0.0f;

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
    
    Vector2 currentAimVector;
    IAimStrategy* currentAimStrategy;

    std::vector<std::unique_ptr<IBuff>> personalBuffs;

    void TickTimers(float deltaTime);
    class Projectile* SpawnLinearProjectile(Vector2 dir, float speed, int damage, float maxFlyTime, bool piercing, Texture2D tex, bool fixedRotation);

public:
    Paladin(
        Vector2 pos,
        CharacterSprites sprites,
        const PaladinDefinition& definition
    );
    virtual ~Paladin();

    virtual void Update(float deltaTime) override;
    virtual void Draw() override;
    virtual void UpdateInactive(float deltaTime);
    virtual void DrawInactive();
    virtual bool IsDoingUltimate() const { return false; }
    
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
    virtual void ApplyKnockback(Vector2 dir, float force);
    
    // Aegis Shield Methods
    bool IsInvulnerable() const { return isInvulnerable; }
    void SetInvulnerable(bool val) { isInvulnerable = val; }
    float GetInvulnerabilityTimer() const { return invulnerabilityTimer; }
    void SetInvulnerabilityTimer(float val) { invulnerabilityTimer = val; }
    
    void SetAimTarget(Vector2 target) { if (!isParrying) aimTarget = target; }
    void UpdateAim(Vector2 rawMouseWorld);
    Vector2 GetAimTarget() const { return aimTarget; }
    
    void SetLockedEnemy(class Enemy* target) { lockedEnemy = target; }
    class Enemy* GetLockedEnemy() const { return lockedEnemy; }
    
    float GetCurrentAimAngle() const { return currentAimAngle; }
    void SetCurrentAimAngle(float angle) { currentAimAngle = angle; }
    
    float GetTargetAimAngle() const { return targetAimAngle; }
    void SetTargetAimAngle(float angle) { targetAimAngle = angle; }
    
    Vector2 GetCurrentAimVector() const { return currentAimVector; }
    void SetCurrentAimStrategy(IAimStrategy* strategy) { currentAimStrategy = strategy; }
    IAimStrategy* GetCurrentAimStrategy() const { return currentAimStrategy; }
    
    void SetTeamManager(TeamManager* manager) { teamManager = manager; }
    TeamManager* GetTeamManager() const { return teamManager; }

    void AddPersonalBuff(std::unique_ptr<IBuff> buff) {
        if (buff) {
            buff->OnApply(this);
            personalBuffs.push_back(std::move(buff));
        }
    }
    const std::vector<std::unique_ptr<IBuff>>& GetPersonalBuffs() const { return personalBuffs; }
    
    // Check if a buff exists (useful for DualWield check in Lance)
    template<typename T>
    bool HasPersonalBuff() const {
        for (const auto& buff : personalBuffs) {
            if (dynamic_cast<T*>(buff.get())) return true;
        }
        return false;
    }

    void ChangeState(IPlayerState* newState);
    virtual void Attack();
    
    virtual void UseSkill() = 0;
    virtual void UseUltimate() = 0;
    virtual void ExecuteUltimateAction() = 0;
    
    Vector2 GetWeaponPivot() const;
    void SetWeapon(IAttackStrategy* weapon) { 
        currentWeapon = weapon; 
        if (currentWeapon) currentWeapon->SetOwner(this);
    }
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

    // Ultimate cooldown
    static constexpr float ULTIMATE_COOLDOWN_MAX = 5.0f;
    float GetUltimateCooldownTimer() const { return ultimateCooldownTimer; }
    void ResetUltimateCooldown() { ultimateCooldownTimer = ULTIMATE_COOLDOWN_MAX; }
    void TickUltimateCooldown(float dt) { if (ultimateCooldownTimer > 0.0f) ultimateCooldownTimer -= dt; }
    
    virtual bool IsWeaponVisible() const { return true; }
    
    const UltimateIntroData& GetIntroData() const { return introData; }
    
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
    int GetNumFrames() const { return numFrames; }
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
