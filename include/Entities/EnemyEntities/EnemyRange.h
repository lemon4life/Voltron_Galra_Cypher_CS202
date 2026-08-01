#pragma once

#include "Entities/Enemy.h"

#include <memory>

class EnemyRangeShootingState;

class EnemyRange : public Enemy {
private:
    std::unique_ptr<EnemyRangeShootingState> shootingState;
    ILevelLineOfSightQuery& lineOfSightQuery;

public:
    EnemyRange(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess,
        ILevelLineOfSightQuery& lineOfSightQuery
    );
    ~EnemyRange() override;

    void Update(float deltaTime) override;
    void Draw() override;
    EnemyPathGoal GetPathGoal() const override;

    EnemyRangeShootingState* GetShootingState();

    bool IsWithinShootingDistance(Vector2 targetPosition) const;
    bool IsBeyondDisengageDistance(Vector2 targetPosition) const;
    float GetProjectileSpeed() const;
    float GetProjectileLifetime() const;
    float GetProjectileRadius() const;
    const ILevelLineOfSightQuery& GetLineOfSightQuery() const { return lineOfSightQuery; }

};
