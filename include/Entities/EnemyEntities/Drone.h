#pragma once

#include "Entities/Enemy.h"
#include <memory>

class DroneState;

class Drone : public Enemy {
private:
    std::unique_ptr<DroneState> activeState;
    ILevelLineOfSightQuery& lineOfSightQuery;
    float attackCooldown;
    float hoverTime = 0.0f;
    
public:
    Drone(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess,
        ILevelLineOfSightQuery& lineOfSightQuery
    );
    ~Drone() override;
    
    void Update(float deltaTime) override;
    void Draw() override;
    
    void Attack();
    
    float GetAttackCooldown() const { return attackCooldown; }
    void ResetAttackCooldown() { attackCooldown = 4.0f; }
    void DecreaseCooldown(float deltaTime) { 
        if (attackCooldown > 0.0f) attackCooldown -= deltaTime; 
    }
    
    const ILevelLineOfSightQuery& GetLineOfSightQuery() const { return lineOfSightQuery; }
};
