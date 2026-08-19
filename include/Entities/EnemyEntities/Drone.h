#pragma once

#include "Entities/Enemy.h"
#include <memory>

class DroneMovingState;
class DroneIdleState;

class Drone : public Enemy {
private:
    std::unique_ptr<DroneMovingState> movingState;
    std::unique_ptr<DroneIdleState> droneIdleState;
    ILevelLineOfSightQuery& lineOfSightQuery;
    
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
    Rectangle GetBoundingBox() const override;

    bool Attack();
    void TickAttackCooldown(float deltaTime, float rate = 1.0f);

    DroneMovingState* GetMovingState() const {
        return movingState.get();
    }
    DroneIdleState* GetDroneIdleState() const {
        return droneIdleState.get();
    }
    const ILevelLineOfSightQuery& GetLineOfSightQuery() const { return lineOfSightQuery; }
};
