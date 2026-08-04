#pragma once


#include "Entities/Enemy.h"

#include <memory>

class EnemyDiverReadyState;
class EnemyDiverLungingState;

class EnemyDiver : public Enemy {
private:
    std::unique_ptr<EnemyDiverReadyState> readyState;
    std::unique_ptr<EnemyDiverLungingState> lungingState;
    ILevelLineOfSightQuery& lineOfSightQuery;
    Vector2 staticEffectPos;

public:
    EnemyDiver(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess,
        ILevelLineOfSightQuery& lineOfSightQuery
    );
    ~EnemyDiver() override;

    void Update(float deltaTime) override;
    void Draw() override;
    EnemyPathGoal GetPathGoal() const override;

    EnemyDiverReadyState* GetReadyState();
    EnemyDiverLungingState* GetLungingState();

    bool CanEnterReadyState() const;
    bool IsWithinClearDiveRange() const;
    bool IsBeyondDisengageDistance(Vector2 targetPosition) const;

    float GetReadyDuration() const;
    float GetReadySpeed() const;
    float GetDiveDuration() const;
    float GetDiveStopDistance() const;
    float GetDiveSpeed() const;
    float GetDiveRecoveryDuration() const;
    float GetCollisionClearanceRadius() const;
    float GetPreferredPathGoalDistance() const override;
    bool IsValidPathGoalPosition(
        Vector2 candidatePosition,
        const Paladin& target
    ) const override;
    const ILevelLineOfSightQuery& GetLineOfSightQuery() const { return lineOfSightQuery; }

};
