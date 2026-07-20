#pragma once

#include "Entities/PathfindingEnemy.h"

#include <memory>

class EnemyRangeShootingState;

class EnemyRange : public PathfindingEnemy {
private:
    std::unique_ptr<EnemyRangeShootingState> shootingState;
    ILevelLineOfSightQuery* lineOfSightQuery;

public:
    EnemyRange(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess* removalAccess,
        IEnemyPathAccess* pathAccess,
        ILevelLineOfSightQuery* lineOfSightQuery
    );
    ~EnemyRange() override;

    void Update(float deltaTime) override;
    void Draw() override;

    EnemyRangeShootingState* GetShootingState();

    bool IsWithinShootingDistance(Vector2 targetPosition) const;
    bool IsBeyondDisengageDistance(Vector2 targetPosition) const;
    float GetProjectileSpeed() const;
    float GetProjectileLifetime() const;
    float GetProjectileRadius() const;
    float GetMaxPredictionTime() const;
    ILevelLineOfSightQuery* GetLineOfSightQuery() const { return lineOfSightQuery; }

};
