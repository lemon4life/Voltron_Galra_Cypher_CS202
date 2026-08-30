#pragma once

#include "raylib.h"

#include <functional>

struct CollisionMovementResult {
    Vector2 appliedDisplacement = { 0.0f, 0.0f };
    bool blockedX = false;
    bool blockedY = false;

    /// Reports whether collision resolution encountered blocking geometry.
    bool HitObstacle() const {
        return blockedX || blockedY;
    }
};

namespace CollisionMovement {
    using CollisionQuery = std::function<bool(Rectangle)>;

    /// Resolves movement one axis at a time so an actor can slide along blocking geometry.
    CollisionMovementResult ResolveSlide(
        Rectangle collisionBox,
        Vector2 desiredDisplacement,
        const CollisionQuery& isBlocked
    );

    /// Clamps movement at the first blocking contact instead of allowing penetration.
    CollisionMovementResult ResolveStop(
        Rectangle collisionBox,
        Vector2 desiredDisplacement,
        const CollisionQuery& isBlocked
    );
}
