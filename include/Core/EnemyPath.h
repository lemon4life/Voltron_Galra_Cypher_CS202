#pragma once
#include "raylib.h"

#include <deque>

class EnemyPathFinding {
private:
    bool usePathFinding = false;
    std::deque<Vector2> targetPosition;
public:
    bool IsPathFinding() const { return usePathFinding; }
    void SetPathFinding(bool value) { usePathFinding = value; }

    void ClearTargetPosition() { targetPosition.clear(); }
    void PopTarget() { if (!targetPosition.empty()) targetPosition.pop_front(); }
    void AddTargetPosition(Vector2 position) { targetPosition.push_back(position); }
    Vector2 FirstTargetPosition() const { return (targetPosition.size() > 0 ? targetPosition[0] : Vector2{-1.f,-1.f} ); }
    bool HasTargetPosition() const { return targetPosition.size() > 0; }
    int TargetPositionCount() const { return (int)targetPosition.size(); }
};
