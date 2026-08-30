#pragma once
#include "Entities/GameObject.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "Entities/Projectile.h"
#include <vector>

class GameManager; // Forward declare
class TeamManager; // Forward declare

class Rover : public GameObject {
private:
    Paladin* owner;
    TeamManager* teamManager;
    int maxHealth;
    int health;
    float fireCooldown;
    float fireTimer;
    float projectileSpeed;
    float aggroRange;
    Vector2 currentVelocity = {0.0f, 0.0f};

    bool isLanding = true;
    bool isFlyingOut = false;
    bool isRemoved = false;
    bool facingLeft = false;
    Vector2 targetOffset = { 30.0f, -30.0f };
    float hoverTimer = 0.0f;
    float attackCooldown = 1.5f;
    
    bool isHealing = false;
    int healFrame = 0;
    float healFrameTimer = 0.0f;

    Texture2D sprite;
    
public:
    /// Creates a Rover instance from the supplied configuration.
    Rover(Vector2 startPos, Paladin* owner, TeamManager* team);
    
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    
    /// Applies incoming damage after this object handles defenses and state-specific rules.
    void TakeDamage(int damage);
    /// Reports whether the dead condition is satisfied.
    bool IsDead() const { return health <= 0 && isRemoved; }
    /// Implements the heal behavior for this component.
    void Heal();
    
    /// Returns the current owner.
    Paladin* GetOwner() const { return owner; }
};
