#pragma once

#include "Entities/Enemy.h"

#include <deque>

class PathfindingEnemy : public Enemy {
private:
    bool usePathFinding = false;
    std::deque<Vector2> targetPositions;
    IEnemyPathAccess* pathAccess;

public:
    PathfindingEnemy(
        Vector2 position,
        TeamManager* targetTeam,
        int maxHealth,
        float speed,
        int damage,
        float attackCooldown,
        IEntityRemovalAccess* removalAccess,
        IEnemyPathAccess* pathAccess
    );
    ~PathfindingEnemy() override;

    void StartPathFinding();
    void EndPathFinding();
    bool IsPathFinding() const { return usePathFinding; }

    void ClearTargetPosition() { targetPositions.clear(); }
    void PopTarget() {
        if (!targetPositions.empty()) targetPositions.pop_front();
    }
    void AddTargetPosition(Vector2 position) {
        targetPositions.push_back(position);
    }
    Vector2 FirstTargetPosition() const {
        return targetPositions.empty()
            ? Vector2{ -1.0f, -1.0f }
            : targetPositions.front();
    }
    bool HasTargetPosition() const { return !targetPositions.empty(); }
};
