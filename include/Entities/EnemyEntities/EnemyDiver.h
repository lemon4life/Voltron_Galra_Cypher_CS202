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

    /// Calculates attack effect origin.
    Vector2 CalculateAttackEffectOrigin() const;
    /// Returns the current attack effect end.
    Vector2 GetAttackEffectEnd() const;

public:
    /// Creates a EnemyDiver instance from the supplied configuration.
    EnemyDiver(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess,
        ILevelLineOfSightQuery& lineOfSightQuery
    );
    /// Releases resources owned by this EnemyDiver instance.
    ~EnemyDiver() override;

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;

    /// Returns the current ready state.
    EnemyDiverReadyState* GetReadyState();
    /// Returns the current lunging state.
    EnemyDiverLungingState* GetLungingState();

    /// Reports whether this component can perform enter ready state.
    bool CanEnterReadyState() const;
    /// Reports whether the within clear dive range condition is satisfied.
    bool IsWithinClearDiveRange() const;
    /// Reports whether the beyond disengage distance condition is satisfied.
    bool IsBeyondDisengageDistance(Vector2 targetPosition) const;

    /// Returns the current ready duration.
    float GetReadyDuration() const;
    /// Returns the current ready speed.
    float GetReadySpeed() const;
    /// Returns the current dive duration.
    float GetDiveDuration() const;
    /// Returns the current dive stop distance.
    float GetDiveStopDistance() const;
    /// Returns the current minimum player distance.
    float GetMinimumPlayerDistance() const;
    /// Returns the current dive speed.
    float GetDiveSpeed() const;
    /// Returns the current dive recovery duration.
    float GetDiveRecoveryDuration() const;
    /// Returns the current collision clearance radius.
    float GetCollisionClearanceRadius() const;
    /// Begins attack preparation.
    void BeginAttackPreparation(Vector2 direction);
    /// Advances attack preparation.
    void AdvanceAttackPreparation(float deltaTime);
    /// Finishes attack preparation.
    void EndAttackPreparation();
    /// Begins attack effect.
    void BeginAttackEffect();
    /// Finishes attack effect.
    void EndAttackEffect();
    /// Returns the current locked attack direction.
    Vector2 GetLockedAttackDirection() const {
        return lockedAttackDirection;
    }
    /// Implements the does attack hit behavior for this component.
    bool DoesAttackHit(Rectangle targetBounds) const;
    /// Returns the current attack contact position.
    Vector2 GetAttackContactPosition(Rectangle targetBounds) const;
    /// Returns the current line of sight query.
    const ILevelLineOfSightQuery& GetLineOfSightQuery() const { return lineOfSightQuery; }

};
