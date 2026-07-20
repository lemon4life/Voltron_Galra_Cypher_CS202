#pragma once
#include "raylib.h"

#include <deque>

class IEnemyPathAccess;

class EnemyPathFinding {
private:
    bool usePathFinding = false;
    std::deque<Vector2> targetPosition;
    IEnemyPathAccess* pathAccess;
public:
    explicit EnemyPathFinding(IEnemyPathAccess* pathAccess)
        : pathAccess(pathAccess) {}

    bool IsPathFinding() const { return usePathFinding; }
    void SetPathFinding(bool value) { usePathFinding = value; }
    IEnemyPathAccess* GetPathAccess() const { return pathAccess; }

    void ClearTargetPosition() { targetPosition.clear(); }
    void PopTarget() { if (!targetPosition.empty()) targetPosition.pop_front(); }
    void AddTargetPosition(Vector2 position) { targetPosition.push_back(position); }
    Vector2 FirstTargetPosition() const { return (targetPosition.size() > 0 ? targetPosition[0] : Vector2{-1.f,-1.f} ); }
    bool HasTargetPosition() const { return targetPosition.size() > 0; }
    int TargetPositionCount() const { return (int)targetPosition.size(); }
};
