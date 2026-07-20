#pragma once

#include "Entities/PathfindingEnemy.h"

#include <memory>

class EnemyDiverReadyState;
class EnemyDiverLungingState;

class EnemyDiver : public PathfindingEnemy {
private:
    std::unique_ptr<EnemyDiverReadyState> readyState;
    std::unique_ptr<EnemyDiverLungingState> lungingState;
    ILevelLineOfSightQuery* lineOfSightQuery;

public:
    EnemyDiver(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess* removalAccess,
        IEnemyPathAccess* pathAccess,
        ILevelLineOfSightQuery* lineOfSightQuery
    );
    ~EnemyDiver() override;

    void Update(float deltaTime) override;
    void Draw() override;

    EnemyDiverReadyState* GetReadyState();
    EnemyDiverLungingState* GetLungingState();

    bool CanEnterReadyState() const;
    bool IsWithinClearDiveRange() const;

    float GetReadyDuration() const;
    float GetReadySpeed() const;
    float GetDiveDuration() const;
    float GetDiveStopDistance() const;
    float GetDiveSpeed() const;
    float GetDiveRecoveryDuration() const;
    float GetCollisionClearanceRadius() const;
    ILevelLineOfSightQuery* GetLineOfSightQuery() const { return lineOfSightQuery; }

};
