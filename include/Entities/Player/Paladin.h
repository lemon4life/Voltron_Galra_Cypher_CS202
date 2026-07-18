#pragma once
#include "Entities/Character.h"
#include "Entities/Player/PlayerState.h"
#include "Entities/Player/PlayerDashState.h"
#include "Entities/Player/PlayerAttackState.h"
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
};

class TeamManager;

class Paladin : public Character {
protected:
    IPlayerState* currentState;
    IAttackStrategy* currentWeapon;
    
    CharacterSprites sprites;
    TeamManager* teamManager;

    // Animation specific
    int currentFrame;
    float frameTimer;
    float frameDuration;
    bool facingLeft;
    int numFrames; // Default 4

    // State instances
    PlayerIdleState idleState;
    PlayerRunState runState;
    PlayerDashState dashState;
    PlayerAttackState attackState;

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
    Vector2 lastMoveDir;
    float footstepTimer;
    Vector2 aimTarget;

public:
    Paladin(Vector2 pos, CharacterSprites sprites, int maxHp, float maxEx);
    virtual ~Paladin();

    void Update(float deltaTime) override;
    void Draw() override;
    
    void SetAimTarget(Vector2 target) { aimTarget = target; }
    Vector2 GetAimTarget() const { return aimTarget; }
    
    void SetTeamManager(TeamManager* manager) { teamManager = manager; }
    TeamManager* GetTeamManager() const { return teamManager; }

    void ChangeState(IPlayerState* newState);
    void Attack();
    Vector2 GetWeaponPivot() const;
    void SetWeapon(IAttackStrategy* weapon) { currentWeapon = weapon; }
    IAttackStrategy* GetCurrentWeapon() const { return currentWeapon; }
    
    virtual void TakeDamage(int amount);
    void OnHitEnemy(int damage);
    void ResetStats();
    void UpdateFootsteps(float dt);

    Rectangle GetBoundingBox() const override;
    bool CheckCollision(const std::vector<GameObject*>& entities) const;

    // Getters
    int GetHealth() const { return health; }
    int GetMaxHealth() const { return maxHealth; }
    float GetGhostHp() const { return ghostHp; }
    float GetExEnergy() const { return exEnergy; }
    float GetMaxExEnergy() const { return maxExEnergy; }
    
    PlayerIdleState* GetIdleState() { return &idleState; }
    PlayerRunState* GetRunState() { return &runState; }
    PlayerDashState* GetDashState() { return &dashState; }
    PlayerAttackState* GetAttackState() { return &attackState; }
    
    Texture2D GetIdleTexture() const;
    Texture2D GetRunTexture() const;

    // Animation helpers
    void UpdateAnimation(float deltaTime);
    void SetNumFrames(int frames) { numFrames = frames; }
    void ResetAnimation() { currentFrame = 0; frameTimer = 0.0f; }
    void SetFacingLeft(bool left) { facingLeft = left; }
    bool IsFacingLeft() const { return facingLeft; }

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
};
