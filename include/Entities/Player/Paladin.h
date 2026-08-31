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
#include "Core/MissionSaveData.h"

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

struct AttachedEffect {
    Texture2D texture;
    int numFrames;
    int currentFrame;
    float lifetime;
    float maxLifetime;
};

class TeamManager;

class Paladin : public Character {
protected:
    IPlayerState* currentState;
    std::unique_ptr<IAttackStrategy> currentWeapon;
    
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
    
    float displayedHp;
    float displayedExEnergy;
    
    float skillCost = 0.0f;
    Rectangle hudPortraitSlice = {0.0f, 0.0f, 0.0f, 0.0f};

    // Active Skill State & Continuous EX Depletion
    bool isSkillActive = false;
    float activeSkillDuration = 0.0f;
    float activeSkillTimer = 0.0f;
    float skillInitialEx = 0.0f;

    // Ultimate cooldown (separate from EX — gated by Quintessence)
    float ultimateCooldownTimer = 0.0f;
    
    // Aegis Shield Mechanic
    bool isInvulnerable = false;
    float invulnerabilityTimer = 0.0f;

    // Dash mechanic properties
    float dashCooldown;
    float attackCooldown;
    float baseAttackCooldown = 0.5f;
    float dashTimer;

    // Unified Paladin Level Progression
    int paladinLevel = 1;

    // Upgraded Stat Scalars
    float hpScalar = 1.0f;
    float attackCooldownScalar = 1.0f;
    float speedScalar = 1.0f;
    float damageScalar = 1.0f;

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
    ObjectId lockedEnemyId = INVALID_OBJECT_ID;
    float currentAimAngle;
    float targetAimAngle;
    
    Vector2 currentAimVector;
    IAimStrategy* currentAimStrategy;

    std::vector<std::unique_ptr<IBuff>> personalBuffs;
    std::vector<AttachedEffect> attachedEffects;

    /// Advances timers.
    void TickTimers(float deltaTime);
    /// Spawns linear projectile.
    class Projectile* SpawnLinearProjectile(Vector2 dir, float speed, int damage, float maxFlyTime, bool piercing, Texture2D tex, bool fixedRotation);

public:
    /// Creates a Paladin instance from the supplied configuration.
    Paladin(
        Vector2 pos,
        CharacterSprites sprites,
        const PaladinDefinition& definition
    );
    /// Releases resources owned by this Paladin instance.
    virtual ~Paladin();

    /// Advances this component's state for the current frame.
    virtual void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    virtual void Draw() override;
    /// Updates inactive.
    virtual void UpdateInactive(float deltaTime);
    /// Renders inactive.
    virtual void DrawInactive();
    /// Reports whether the doing ultimate condition is satisfied.
    virtual bool IsDoingUltimate() const { return false; }
    
    /// Adds attached effect.
    void AddAttachedEffect(Texture2D tex, int frames, float lifetime);
    
    /// Updates the stored parrying.
    void SetParrying(bool parry);
    /// Returns the current consecutive parries.
    int GetConsecutiveParries() const { return consecutiveParries; }
    /// Implements the increment parry count behavior for this component.
    void IncrementParryCount() { 
        consecutiveParries++; 
        if (consecutiveParries >= 3 && isParrying) {
            ChangeState(&idleState);
        }
    }
    /// Resets parry count.
    void ResetParryCount() { consecutiveParries = 0; }
    /// Reports whether the parrying condition is satisfied.
    bool IsParrying() const { return isParrying; }
    /// Reports whether this component can perform parry attack.
    bool CanParryAttack(Vector2 attackerPos) const;
    /// Triggers parry success.
    void TriggerParrySuccess(GameObject* attacker);
    /// Applies knockback.
    virtual void ApplyKnockback(Vector2 dir, float force);
    
    // Aegis Shield Methods
    /// Reports whether the invulnerable condition is satisfied.
    bool IsInvulnerable() const { return isInvulnerable; }
    /// Updates the stored invulnerable.
    void SetInvulnerable(bool val) { isInvulnerable = val; }
    /// Returns the current invulnerability timer.
    float GetInvulnerabilityTimer() const { return invulnerabilityTimer; }
    /// Updates the stored invulnerability timer.
    void SetInvulnerabilityTimer(float val) { invulnerabilityTimer = val; }
    
    /// Updates the stored aim target.
    void SetAimTarget(Vector2 target) { if (!isParrying) aimTarget = target; }
    /// Updates aim.
    void UpdateAim(Vector2 rawMouseWorld);
    /// Returns the current aim target.
    Vector2 GetAimTarget() const { return aimTarget; }
    
    /// Updates the stored locked enemy.
    void SetLockedEnemy(class Enemy* target);
    /// Returns the current locked enemy.
    class Enemy* GetLockedEnemy() const;
    
    /// Returns the current current aim angle.
    float GetCurrentAimAngle() const { return currentAimAngle; }
    /// Updates the stored current aim angle.
    void SetCurrentAimAngle(float angle) { currentAimAngle = angle; }
    
    /// Returns the current target aim angle.
    float GetTargetAimAngle() const { return targetAimAngle; }
    /// Updates the stored target aim angle.
    void SetTargetAimAngle(float angle) { targetAimAngle = angle; }
    
    /// Returns the current current aim vector.
    Vector2 GetCurrentAimVector() const { return currentAimVector; }
    /// Updates the stored current aim strategy.
    void SetCurrentAimStrategy(IAimStrategy* strategy) { currentAimStrategy = strategy; }
    /// Returns the current current aim strategy.
    IAimStrategy* GetCurrentAimStrategy() const { return currentAimStrategy; }
    
    /// Updates the stored team manager.
    void SetTeamManager(TeamManager* manager) { teamManager = manager; }
    /// Returns the current team manager.
    TeamManager* GetTeamManager() const { return teamManager; }

    /// Adds personal buff.
    void AddPersonalBuff(std::unique_ptr<IBuff> buff) {
        if (buff) {
            buff->OnApply(this);
            personalBuffs.push_back(std::move(buff));
        }
    }
    /// Returns the current personal buffs.
    const std::vector<std::unique_ptr<IBuff>>& GetPersonalBuffs() const { return personalBuffs; }
    
    // Check if a buff exists (useful for DualWield check in Lance)
    template<typename T>
    /// Reports whether this component has personal buff.
    bool HasPersonalBuff() const {
        for (const auto& buff : personalBuffs) {
            if (dynamic_cast<T*>(buff.get())) return true;
        }
        return false;
    }

    /// Leaves the current state, switches ownership, and enters the replacement state.
    void ChangeState(IPlayerState* newState);
    /// Starts this attack behavior when its current conditions allow it.
    virtual void Attack();
    
    /// Activates skill.
    virtual void UseSkill() = 0;
    /// Activates ultimate.
    virtual void UseUltimate() = 0;
    /// Executes the gameplay effect after the Ultimate introduction finishes.
    virtual void ExecuteUltimateAction() = 0;
    
    /// Returns the current weapon pivot.
    Vector2 GetWeaponPivot() const;
    /// Updates the stored weapon.
    void SetWeapon(std::unique_ptr<IAttackStrategy> weapon) {
        currentWeapon = std::move(weapon);
        if (currentWeapon) currentWeapon->SetOwner(this);
    }
    /// Returns the current current weapon.
    IAttackStrategy* GetCurrentWeapon() const { return currentWeapon.get(); }
    
    /// Applies incoming damage after this object handles defenses and state-specific rules.
    virtual void TakeDamage(int amount);
    /// Handles the hit enemy event.
    void OnHitEnemy(int damage);
    /// Resets stats.
    void ResetStats();
    /// Updates footsteps.
    void UpdateFootsteps(float dt);

    /// Returns the current bounding box.
    Rectangle GetBoundingBox() const override;
    /// Returns the current collision box.
    Rectangle GetCollisionBox() const override;
    /// Moves against level.
    Vector2 MoveAgainstLevel(Vector2 desiredDisplacement);
    /// Checks collision.
    bool CheckCollision(const std::vector<GameObject*>& entities) const;

    // Getters
    /// Returns the current health.
    int GetHealth() const { return health; }
    /// Returns the current max health.
    int GetMaxHealth() const { return maxHealth; }
    /// Implements the heal behavior for this component.
    void Heal(int amount) {
        health += amount;
        if (health > maxHealth) health = maxHealth;
    }
    /// Returns the current paladin id.
    PaladinId GetPaladinId() const { return paladinId; }
    /// Returns the current ghost hp.
    float GetGhostHp() const { return ghostHp; }
    /// Returns the current displayed hp.
    float GetDisplayedHp() const { return displayedHp; }
    /// Returns the current displayed ex energy.
    float GetDisplayedExEnergy() const { return displayedExEnergy; }
    /// Returns the current ex energy.
    float GetExEnergy() const { return exEnergy; }
    /// Returns the current max ex energy.
    float GetMaxExEnergy() const { return maxExEnergy; }
    /// Adds ex energy.
    void AddExEnergy(float amount);
    /// Returns the current skill cost.
    float GetSkillCost() const { return skillCost; }
    /// Returns the current hud portrait slice.
    Rectangle GetHudPortraitSlice() const { return hudPortraitSlice; }

    /// Reports whether the skill active condition is satisfied.
    bool IsSkillActive() const { return isSkillActive; }
    /// Returns the current active skill timer.
    float GetActiveSkillTimer() const { return activeSkillTimer; }
    /// Returns the current active skill duration.
    float GetActiveSkillDuration() const { return activeSkillDuration; }
    /// Activates skill.
    void ActivateSkill(float duration = 5.0f);

    // Unified Paladin Level Progression
    static constexpr int MAX_PALADIN_LEVEL = 5;
    /// Returns the current paladin level.
    int GetPaladinLevel() const { return paladinLevel; }
    /// Returns the current max paladin level.
    static int GetMaxPaladinLevel() { return MAX_PALADIN_LEVEL; }
    /// Reports whether the max level condition is satisfied.
    bool IsMaxLevel() const { return paladinLevel >= MAX_PALADIN_LEVEL; }
    /// Returns the current upgrade cost.
    int GetUpgradeCost() const { return 5 * paladinLevel; }
    /// Reports whether this component can perform level up.
    bool CanLevelUp(int currentCoins) const { return !IsMaxLevel() && currentCoins >= GetUpgradeCost(); }
    /// Implements the level up behavior for this component.
    bool LevelUp();
    /// Returns the current paladin progress.
    float GetPaladinProgress() const { return (MAX_PALADIN_LEVEL > 1) ? ((float)(paladinLevel - 1) / (float)(MAX_PALADIN_LEVEL - 1)) : 1.0f; }

    /// Recalculates stats.
    void RecalculateStats();

    /// Returns the current hp scalar.
    float GetHpScalar() const { return hpScalar; }
    /// Returns the current attack cooldown scalar.
    float GetAttackCooldownScalar() const { return attackCooldownScalar; }
    /// Returns the current speed scalar.
    float GetSpeedScalar() const { return speedScalar; }
    /// Returns the current damage scalar.
    float GetDamageScalar() const { return damageScalar; }
    /// Returns the current base attack cooldown.
    float GetBaseAttackCooldown() const { return baseAttackCooldown; }

    // Ultimate cooldown
    static constexpr float ULTIMATE_COOLDOWN_MAX = 5.0f;
    /// Returns the current ultimate cooldown timer.
    float GetUltimateCooldownTimer() const { return ultimateCooldownTimer; }
    /// Resets ultimate cooldown.
    void ResetUltimateCooldown() { ultimateCooldownTimer = ULTIMATE_COOLDOWN_MAX; }
    /// Advances ultimate cooldown.
    void TickUltimateCooldown(float dt) { if (ultimateCooldownTimer > 0.0f) ultimateCooldownTimer -= dt; }
    
    /// Reports whether the weapon visible condition is satisfied.
    virtual bool IsWeaponVisible() const { return true; }
    
    /// Returns the current intro data.
    const UltimateIntroData& GetIntroData() const { return introData; }
    /// Captures persistent progression and combat resources at a safe checkpoint.
    SavedPaladinState CaptureCheckpointState() const;
    /// Restores persistent state and resumes from a stable idle/down state.
    void RestoreCheckpointState(const SavedPaladinState& saved);
    
    /// Returns the current idle state.
    PlayerIdleState* GetIdleState() { return &idleState; }
    /// Returns the current run state.
    PlayerRunState* GetRunState() { return &runState; }
    /// Returns the current parry state.
    PlayerParryState* GetParryState() { return &parryState; }
    /// Returns the current dash state.
    PlayerDashState* GetDashState() { return &dashState; }
    /// Returns the current attack state.
    PlayerAttackState* GetAttackState() { return &attackState; }
    /// Returns the current down state.
    PlayerDownState* GetDownState() { return &downState; }
    
    /// Returns the current idle texture.
    Texture2D GetIdleTexture() const;
    /// Returns the current run texture.
    Texture2D GetRunTexture() const;
    /// Returns the current dash front texture.
    Texture2D GetDashFrontTexture() const;
    /// Returns the current dash back texture.
    Texture2D GetDashBackTexture() const;
    /// Returns the current parry texture.
    Texture2D GetParryTexture() const;
    /// Returns the current down texture.
    Texture2D GetDownTexture() const;

    // Animation helpers
    /// Updates animation.
    void UpdateAnimation(float deltaTime);
    /// Updates the stored num frames.
    void SetNumFrames(int frames) { numFrames = frames; }
    /// Returns the current num frames.
    int GetNumFrames() const { return numFrames; }
    /// Resets animation.
    void ResetAnimation() { currentFrame = 0; frameTimer = 0.0f; }
    /// Updates the stored facing left.
    void SetFacingLeft(bool left) { facingLeft = left; }
    /// Reports whether the facing left condition is satisfied.
    bool IsFacingLeft() const { return facingLeft; }
    // Returns the source rectangle for the current animation frame (used by particle effects)
    /// Returns the current current source rect.
    Rectangle GetCurrentSourceRect() const {
        float fw = (float)texture.width / (float)numFrames;
        float fh = (float)texture.height;
        return { (float)currentFrame * fw, 0.0f, fw, fh };
    }

    // Dash Getters and Setters
    /// Returns the current dash cooldown.
    float GetDashCooldown() const { return dashCooldown; }
    /// Returns the current attack cooldown.
    float GetAttackCooldown() const { return attackCooldown; }
    /// Updates the stored attack cooldown.
    void SetAttackCooldown(float val) { attackCooldown = val; }
    /// Updates the stored dash cooldown.
    void SetDashCooldown(float cooldown) { dashCooldown = cooldown; }
    /// Returns the current dash timer.
    float GetDashTimer() const { return dashTimer; }
    /// Updates the stored dash timer.
    void SetDashTimer(float timer) { dashTimer = timer; }
    /// Reports whether the invincible condition is satisfied.
    bool IsInvincible() const { return isInvincible; }
    /// Updates the stored invincible.
    void SetInvincible(bool invincible) { isInvincible = invincible; }
    /// Returns the current last move dir.
    Vector2 GetLastMoveDir() const { return lastMoveDir; }
    /// Updates the stored last move dir.
    void SetLastMoveDir(Vector2 dir) { lastMoveDir = dir; }

    /// Triggers swap parry window.
    void TriggerSwapParryWindow() { swapParryWindowTimer = 0.2f; }
    /// Implements the decrement swap parry window behavior for this component.
    void DecrementSwapParryWindow(float dt) { if (swapParryWindowTimer > 0.0f) swapParryWindowTimer -= dt; }
    /// Reports whether the auto parrying condition is satisfied.
    bool IsAutoParrying() const { return isAutoParry; }
    /// Updates the stored auto parry.
    void SetAutoParry(bool val) { isAutoParry = val; }
    /// Returns the current auto parry duration timer.
    float GetAutoParryDurationTimer() const { return autoParryDurationTimer; }
    /// Calculates and returns decrement auto parry duration.
    void DecrementAutoParryDuration(float dt) { if (autoParryDurationTimer > 0.0f) autoParryDurationTimer -= dt; }
    /// Resets auto parry duration.
    void ResetAutoParryDuration() { autoParryDurationTimer = 1.0f; }

    /// Returns the current render offset y.
    float GetRenderOffsetY() const { return renderOffsetY; }
    /// Updates the stored render offset y.
    void SetRenderOffsetY(float offset) { renderOffsetY = offset; }
};
