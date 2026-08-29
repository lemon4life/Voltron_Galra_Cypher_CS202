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
    Texture2D attackNotification = {};
    Vector2 attackEffectStart = { 0.0f, 0.0f };
    Vector2 attackEffectEnd = { 0.0f, 0.0f };
    Vector2 lockedAttackDirection = { 1.0f, 0.0f };
    float attackTelegraphElapsed = 0.0f;
    bool attackTelegraphActive = false;

    Vector2 CalculateAttackEffectOrigin() const;
    Vector2 GetAttackEffectEnd() const;

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

    EnemyDiverReadyState* GetReadyState();
    EnemyDiverLungingState* GetLungingState();

    bool CanEnterReadyState() const;
    bool IsWithinClearDiveRange() const;
    bool IsBeyondDisengageDistance(Vector2 targetPosition) const;

    float GetReadyDuration() const;
    float GetReadySpeed() const;
    float GetDiveDuration() const;
    float GetDiveStopDistance() const;
    float GetMinimumPlayerDistance() const;
    float GetDiveSpeed() const;
    float GetDiveRecoveryDuration() const;
    float GetCollisionClearanceRadius() const;
    void BeginAttackPreparation(Vector2 direction);
    void AdvanceAttackPreparation(float deltaTime);
    void EndAttackPreparation();
    void BeginAttackEffect();
    void EndAttackEffect();
    Vector2 GetLockedAttackDirection() const {
        return lockedAttackDirection;
    }
    bool DoesAttackHit(Rectangle targetBounds) const;
    Vector2 GetAttackContactPosition(Rectangle targetBounds) const;
    const ILevelLineOfSightQuery& GetLineOfSightQuery() const { return lineOfSightQuery; }

};
