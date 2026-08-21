#pragma once

#include "raylib.h"

#include <functional>

struct CollisionMovementResult {
    Vector2 appliedDisplacement = { 0.0f, 0.0f };
    bool blockedX = false;
    bool blockedY = false;

    bool HitObstacle() const {
        return blockedX || blockedY;
    }
};

namespace CollisionMovement {
    using CollisionQuery = std::function<bool(Rectangle)>;

    CollisionMovementResult ResolveSlide(
        Rectangle collisionBox,
        Vector2 desiredDisplacement,
        const CollisionQuery& isBlocked
    );

    CollisionMovementResult ResolveStop(
        Rectangle collisionBox,
        Vector2 desiredDisplacement,
        const CollisionQuery& isBlocked
    );
}
