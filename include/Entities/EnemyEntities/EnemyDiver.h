#pragma once

#include "Entities/Enemy.h"
#include "Entities/PathfindingEnemy.h"

#include <memory>

class EnemyDiverReadyState;
class EnemyDiverLungingState;
class LevelManager;

class EnemyDiver : public Enemy, public EnemyPathFinding {
private:
    std::unique_ptr<EnemyDiverReadyState> readyState;
    std::unique_ptr<EnemyDiverLungingState> lungingState;

public:
    EnemyDiver(Vector2 position, TeamManager* targetTeam);

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

    bool CanEnterReadyState(LevelManager* levelManager) const;
    bool IsWithinClearDiveRange(LevelManager* levelManager) const;

    float GetReadyDuration() const;
    float GetReadySpeed() const;
    float GetDiveDuration() const;
    float GetDiveStopDistance() const;
    float GetDiveSpeed() const;
    float GetDiveRecoveryDuration() const;
    float GetCollisionClearanceRadius() const;

    void StartPathFinding();
    void EndPathFinding();
    ILevelLineOfSightQuery* GetLineOfSightQuery() const { return lineOfSightQuery; }

};
