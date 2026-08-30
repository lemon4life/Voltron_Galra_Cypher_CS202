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
    float hoverTime = 0.0f;
    
public:
    /// Creates a Drone instance from the supplied configuration.
    Drone(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess,
        ILevelLineOfSightQuery& lineOfSightQuery
    );
    /// Releases resources owned by this Drone instance.
    ~Drone() override;
    
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    /// Returns the current bounding box.
    Rectangle GetBoundingBox() const override;

    /// Starts this attack behavior when its current conditions allow it.
    bool Attack();
    /// Advances attack cooldown.
    void TickAttackCooldown(float deltaTime, float rate = 1.0f);

    /// Returns the current moving state.
    DroneMovingState* GetMovingState() const {
        return movingState.get();
    }
    /// Returns the current drone idle state.
    DroneIdleState* GetDroneIdleState() const {
        return droneIdleState.get();
    }
    /// Returns the current line of sight query.
    const ILevelLineOfSightQuery& GetLineOfSightQuery() const { return lineOfSightQuery; }
};
