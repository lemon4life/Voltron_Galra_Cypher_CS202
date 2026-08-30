#pragma once

#include "AI/EnemyState.h"
#include "Entities/Enemy.h"

class EnemyChaser : public Enemy {
private:
    std::unique_ptr<EnemyChaserDamageState> damageState;
    float aggroMeter = 0.0f;
    float requiredAggroDuration = 1.0f;

public:
    /// Creates a EnemyChaser instance from the supplied configuration.
    EnemyChaser(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess
    );
    /// Releases resources owned by this EnemyChaser instance.
    ~EnemyChaser() override;

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;

    /// Returns the current damage state.
    EnemyChaserDamageState* GetDamageState();
    /// Reports whether the beyond disengage distance condition is satisfied.
    bool IsBeyondDisengageDistance(Vector2 targetPosition) const;
    /// Reports whether the within aggro range condition is satisfied.
    bool IsWithinAggroRange(Vector2 targetPosition) const;
    /// Reports whether the within stop path finding distance condition is satisfied.
    bool IsWithinStopPathFindingDistance(Vector2 targetPosition) const;
    /// Returns the current damage charge distance.
    float GetDamageChargeDistance() const;
    /// Returns the current damage charge duration.
    float GetDamageChargeDuration() const;

    /// Updates aggro meter.
    void UpdateAggroMeter(bool isNearPlayer, float deltaTime);
    /// Reports whether the aggro ready condition is satisfied.
    bool IsAggroReady() const;
    /// Resets aggro meter.
    void ResetAggroMeter();
};
