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
    Rover(Vector2 startPos, Paladin* owner, TeamManager* team);
    
    void Update(float deltaTime) override;
    void Draw() override;
    
    void TakeDamage(int damage);
    bool IsDead() const { return health <= 0 && isRemoved; }
    void Heal();
    
    Paladin* GetOwner() const { return owner; }
};
