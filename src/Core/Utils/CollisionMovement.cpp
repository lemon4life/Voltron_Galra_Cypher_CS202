#include "Core/Utils/CollisionMovement.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace {
    constexpr float MAX_COLLISION_SUBSTEP = 1.0f;
    constexpr float MOVEMENT_EPSILON = 0.000001f;
    constexpr float CONTACT_SKIN = 0.0005f;
    constexpr int CONTACT_SEARCH_ITERATIONS = 20;
    constexpr int RECOVERY_DISTANCE_DOUBLINGS = 12;

    /// Returns a copy of the rectangle translated by the requested displacement.
    Rectangle Translate(Rectangle rectangle, Vector2 displacement) {
        rectangle.x += displacement.x;
        rectangle.y += displacement.y;
        return rectangle;
    }

    /// Searches for tiny overlap recovery.
    std::optional<Vector2> FindTinyOverlapRecovery(
        Rectangle collisionBox,
        const CollisionMovement::CollisionQuery& isBlocked
    ) {
        if (!isBlocked(collisionBox)) return Vector2{ 0.0f, 0.0f };

        float distance = CONTACT_SKIN;
        for (int attempt = 0;
             attempt < RECOVERY_DISTANCE_DOUBLINGS;
             ++attempt) {
            const std::array<Vector2, 8> offsets = {{
                { -distance, 0.0f },
                { distance, 0.0f },
                { 0.0f, -distance },
                { 0.0f, distance },
                { -distance, -distance },
                { distance, -distance },
                { -distance, distance },
                { distance, distance }
            }};
            for (Vector2 offset : offsets) {
                if (!isBlocked(Translate(collisionBox, offset))) {
                    return offset;
                }
            }
            distance *= 2.0f;
        }
        return std::nullopt;
    }

    /// Moves a collision result slightly away from contact to prevent floating-point overlap.
    float BackOffFromContact(float distance) {
        float absoluteDistance = std::abs(distance);
        if (absoluteDistance <= MOVEMENT_EPSILON) return 0.0f;
        float skin = std::min(CONTACT_SKIN, absoluteDistance);
        return distance - std::copysign(skin, distance);
    }

    /// Clamps movement on one axis against the nearest blocking rectangle.
    float ResolveAxis(
        Rectangle& collisionBox,
        float desiredDistance,
        bool horizontal,
        const CollisionMovement::CollisionQuery& isBlocked,
        bool& blocked
    ) {
        if (!std::isfinite(desiredDistance) ||
            std::abs(desiredDistance) <= MOVEMENT_EPSILON) {
            blocked = !std::isfinite(desiredDistance);
            return 0.0f;
        }

        int stepCount = std::max(
            1,
            (int)std::ceil(
                std::abs(desiredDistance) / MAX_COLLISION_SUBSTEP
            )
        );
        float stepDistance = desiredDistance / (float)stepCount;
        float appliedDistance = 0.0f;

        for (int stepIndex = 0; stepIndex < stepCount; ++stepIndex) {
            Vector2 step = horizontal
                ? Vector2{ stepDistance, 0.0f }
                : Vector2{ 0.0f, stepDistance };
            Rectangle candidate = Translate(collisionBox, step);
            if (!isBlocked(candidate)) {
                collisionBox = candidate;
                appliedDistance += stepDistance;
                continue;
            }

            float clearAmount = 0.0f;
            float blockedAmount = 1.0f;
            for (int iteration = 0;
                 iteration < CONTACT_SEARCH_ITERATIONS;
                 ++iteration) {
                float amount = (clearAmount + blockedAmount) * 0.5f;
                Vector2 partialStep = horizontal
                    ? Vector2{ stepDistance * amount, 0.0f }
                    : Vector2{ 0.0f, stepDistance * amount };
                if (isBlocked(Translate(collisionBox, partialStep))) {
                    blockedAmount = amount;
                } else {
                    clearAmount = amount;
                }
            }

            float contactDistance = BackOffFromContact(
                stepDistance * clearAmount
            );
            if (std::abs(contactDistance) > MOVEMENT_EPSILON) {
                Vector2 contactStep = horizontal
                    ? Vector2{ contactDistance, 0.0f }
                    : Vector2{ 0.0f, contactDistance };
                collisionBox = Translate(collisionBox, contactStep);
                appliedDistance += contactDistance;
            }
            blocked = true;
            break;
        }

        return appliedDistance;
    }

    /// Tests one axis order for sliding and returns the resulting safe displacement.
    CollisionMovementResult ResolveSlideInOrder(
        Rectangle collisionBox,
        Vector2 desiredDisplacement,
        bool horizontalFirst,
        const CollisionMovement::CollisionQuery& isBlocked
    ) {
        CollisionMovementResult result;
        auto resolveHorizontal = [&]() {
            result.appliedDisplacement.x = ResolveAxis(
                collisionBox,
                desiredDisplacement.x,
                true,
                isBlocked,
                result.blockedX
            );
        };
        auto resolveVertical = [&]() {
            result.appliedDisplacement.y = ResolveAxis(
                collisionBox,
                desiredDisplacement.y,
                false,
                isBlocked,
                result.blockedY
            );
        };

        if (horizontalFirst) {
            resolveHorizontal();
            resolveVertical();
        } else {
            resolveVertical();
            resolveHorizontal();
        }
        return result;
    }

    /// Scores how closely a collision-safe displacement follows the requested movement.
    float MovementProgressScore(
        CollisionMovementResult movement,
        Vector2 desiredDisplacement
    ) {
        return movement.appliedDisplacement.x * desiredDisplacement.x +
            movement.appliedDisplacement.y * desiredDisplacement.y;
    }
}

/// Resolves movement one axis at a time so an actor can slide along blocking geometry.
CollisionMovementResult CollisionMovement::ResolveSlide(
    Rectangle collisionBox,
    Vector2 desiredDisplacement,
    const CollisionQuery& isBlocked
) {
    std::optional<Vector2> recovery = FindTinyOverlapRecovery(
        collisionBox,
        isBlocked
    );
    if (!recovery) {
        return {
            { 0.0f, 0.0f },
            desiredDisplacement.x != 0.0f,
            desiredDisplacement.y != 0.0f
        };
    }
    collisionBox = Translate(collisionBox, *recovery);

    CollisionMovementResult horizontalFirst = ResolveSlideInOrder(
        collisionBox,
        desiredDisplacement,
        true,
        isBlocked
    );
    CollisionMovementResult verticalFirst = ResolveSlideInOrder(
        collisionBox,
        desiredDisplacement,
        false,
        isBlocked
    );

    float horizontalScore = MovementProgressScore(
        horizontalFirst,
        desiredDisplacement
    );
    float verticalScore = MovementProgressScore(
        verticalFirst,
        desiredDisplacement
    );
    CollisionMovementResult result =
        verticalScore > horizontalScore + MOVEMENT_EPSILON
        ? verticalFirst
        : horizontalFirst;
    result.appliedDisplacement.x += recovery->x;
    result.appliedDisplacement.y += recovery->y;
    return result;
}

/// Clamps movement at the first blocking contact instead of allowing penetration.
CollisionMovementResult CollisionMovement::ResolveStop(
    Rectangle collisionBox,
    Vector2 desiredDisplacement,
    const CollisionQuery& isBlocked
) {
    CollisionMovementResult result;
    if (!std::isfinite(desiredDisplacement.x) ||
        !std::isfinite(desiredDisplacement.y)) {
        result.blockedX = desiredDisplacement.x != 0.0f;
        result.blockedY = desiredDisplacement.y != 0.0f;
        return result;
    }

    float maximumAxisDistance = std::max(
        std::abs(desiredDisplacement.x),
        std::abs(desiredDisplacement.y)
    );
    if (maximumAxisDistance <= MOVEMENT_EPSILON) return result;

    int stepCount = std::max(
        1,
        (int)std::ceil(maximumAxisDistance / MAX_COLLISION_SUBSTEP)
    );
    Vector2 step = {
        desiredDisplacement.x / (float)stepCount,
        desiredDisplacement.y / (float)stepCount
    };

    for (int stepIndex = 0; stepIndex < stepCount; ++stepIndex) {
        Rectangle candidate = Translate(collisionBox, step);
        if (!isBlocked(candidate)) {
            collisionBox = candidate;
            result.appliedDisplacement.x += step.x;
            result.appliedDisplacement.y += step.y;
            continue;
        }

        float clearAmount = 0.0f;
        float blockedAmount = 1.0f;
        for (int iteration = 0;
             iteration < CONTACT_SEARCH_ITERATIONS;
             ++iteration) {
            float amount = (clearAmount + blockedAmount) * 0.5f;
            Vector2 partialStep = {
                step.x * amount,
                step.y * amount
            };
            if (isBlocked(Translate(collisionBox, partialStep))) {
                blockedAmount = amount;
            } else {
                clearAmount = amount;
            }
        }

        float backedOffAmount = clearAmount;
        float contactDistance = std::sqrt(
            step.x * step.x + step.y * step.y
        ) * clearAmount;
        if (contactDistance > MOVEMENT_EPSILON) {
            backedOffAmount = std::max(
                0.0f,
                clearAmount - CONTACT_SKIN / contactDistance * clearAmount
            );
        }
        Vector2 contactStep = {
            step.x * backedOffAmount,
            step.y * backedOffAmount
        };
        result.appliedDisplacement.x += contactStep.x;
        result.appliedDisplacement.y += contactStep.y;
        result.blockedX = desiredDisplacement.x != 0.0f;
        result.blockedY = desiredDisplacement.y != 0.0f;
        return result;
    }

    return result;
}
