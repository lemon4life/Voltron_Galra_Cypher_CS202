#pragma once

#include "AI/EnemyState.h"
#include "Entities/PathfindingEnemy.h"

class EnemyChaser : public PathfindingEnemy {
public:
    EnemyChaser(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess* removalAccess,
        IEnemyPathAccess* pathAccess
    );
    ~EnemyChaser() override;

    void Update(float deltaTime) override;
    void Draw() override;
};
