#pragma once

#include "Entities/Enemy.h"

#include <memory>

class EnemyRangeShootingState;

class EnemyRange : public Enemy {
private:
    std::unique_ptr<EnemyRangeShootingState> shootingState;
    ILevelLineOfSightQuery& lineOfSightQuery;

public:
    /// Creates a EnemyRange instance from the supplied configuration.
    EnemyRange(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess,
        ILevelLineOfSightQuery& lineOfSightQuery
    );
    /// Releases resources owned by this EnemyRange instance.
    ~EnemyRange() override;

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;

    /// Returns the current shooting state.
    EnemyRangeShootingState* GetShootingState();

    /// Reports whether the within shooting distance condition is satisfied.
    bool IsWithinShootingDistance(Vector2 targetPosition) const;
    /// Reports whether the beyond disengage distance condition is satisfied.
    bool IsBeyondDisengageDistance(Vector2 targetPosition) const;
    /// Returns the current projectile speed.
    float GetProjectileSpeed() const;
    /// Returns the current projectile lifetime.
    float GetProjectileLifetime() const;
    /// Returns the current projectile radius.
    float GetProjectileRadius() const;
    /// Returns the current line of sight query.
    const ILevelLineOfSightQuery& GetLineOfSightQuery() const { return lineOfSightQuery; }

};
