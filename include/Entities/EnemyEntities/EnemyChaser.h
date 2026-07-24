#pragma once

#include "AI/EnemyState.h"
#include "Entities/Enemy.h"

class EnemyChaser : public Enemy {
private:
    std::unique_ptr<EnemyChaserDamageState> damageState;

public:
    EnemyChaser(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess
    );
    ~EnemyChaser() override;

    void Update(float deltaTime) override;
    void Draw() override;

    EnemyChaserDamageState* GetDamageState();
    bool IsBeyondDisengageDistance(Vector2 targetPosition) const;
    bool IsWithinStopPathFindingDistance(Vector2 targetPosition) const;
    float GetDamageChargeDistance() const;
    float GetDamageChargeDuration() const;
};
