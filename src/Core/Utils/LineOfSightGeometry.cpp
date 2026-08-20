#include "Core/Utils/LineOfSightGeometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace {
    constexpr float GEOMETRY_EPSILON = 0.000001f;

    Rectangle NormalizeRectangle(Rectangle rectangle) {
        if (rectangle.width < 0.0f) {
            rectangle.x += rectangle.width;
            rectangle.width = -rectangle.width;
        }
        if (rectangle.height < 0.0f) {
            rectangle.y += rectangle.height;
            rectangle.height = -rectangle.height;
        }
        return rectangle;
    }

    bool IsFinite(Vector2 point) {
        return std::isfinite(point.x) && std::isfinite(point.y);
    }

    bool IsFinite(Rectangle rectangle) {
        return std::isfinite(rectangle.x) &&
            std::isfinite(rectangle.y) &&
            std::isfinite(rectangle.width) &&
            std::isfinite(rectangle.height);
    }

    float PointToRectangleDistanceSquared(
        Vector2 point,
        Rectangle rectangle
    ) {
        float closestX = std::clamp(
            point.x,
            rectangle.x,
            rectangle.x + rectangle.width
        );
        float closestY = std::clamp(
            point.y,
            rectangle.y,
            rectangle.y + rectangle.height
        );
        float deltaX = point.x - closestX;
        float deltaY = point.y - closestY;
        return deltaX * deltaX + deltaY * deltaY;
    }

    float PointToSegmentDistanceSquared(
        Vector2 point,
        Vector2 start,
        Vector2 end
    ) {
        float segmentX = end.x - start.x;
        float segmentY = end.y - start.y;
        float lengthSquared = segmentX * segmentX + segmentY * segmentY;
        if (lengthSquared <= GEOMETRY_EPSILON) {
            float deltaX = point.x - start.x;
            float deltaY = point.y - start.y;
            return deltaX * deltaX + deltaY * deltaY;
        }

        float amount = (
            (point.x - start.x) * segmentX +
            (point.y - start.y) * segmentY
        ) / lengthSquared;
        amount = std::clamp(amount, 0.0f, 1.0f);
        float closestX = start.x + segmentX * amount;
        float closestY = start.y + segmentY * amount;
        float deltaX = point.x - closestX;
        float deltaY = point.y - closestY;
        return deltaX * deltaX + deltaY * deltaY;
    }

    bool SegmentIntersectsRectangle(
        Vector2 start,
        Vector2 end,
        Rectangle rectangle
    ) {
        float minimumAmount = 0.0f;
        float maximumAmount = 1.0f;
        float directionX = end.x - start.x;
        float directionY = end.y - start.y;

        auto clipAxis = [&](float origin, float direction,
                            float minimum, float maximum) {
            if (std::abs(direction) <= GEOMETRY_EPSILON) {
                return origin >= minimum && origin <= maximum;
            }

            float first = (minimum - origin) / direction;
            float second = (maximum - origin) / direction;
            if (first > second) std::swap(first, second);
            minimumAmount = std::max(minimumAmount, first);
            maximumAmount = std::min(maximumAmount, second);
            return minimumAmount <= maximumAmount;
        };

        return clipAxis(
                   start.x,
                   directionX,
                   rectangle.x,
                   rectangle.x + rectangle.width
               ) &&
            clipAxis(
                start.y,
                directionY,
                rectangle.y,
                rectangle.y + rectangle.height
            );
    }
}

bool LineOfSightGeometry::CapsuleIntersectsRectangle(
    Vector2 start,
    Vector2 end,
    float radius,
    Rectangle rectangle
) {
    if (!IsFinite(start) || !IsFinite(end) ||
        !IsFinite(rectangle) || !std::isfinite(radius)) {
        return true;
    }

    rectangle = NormalizeRectangle(rectangle);
    radius = std::max(0.0f, radius);
    if (SegmentIntersectsRectangle(start, end, rectangle)) {
        return true;
    }

    float minimumDistanceSquared = std::min(
        PointToRectangleDistanceSquared(start, rectangle),
        PointToRectangleDistanceSquared(end, rectangle)
    );
    const std::array<Vector2, 4> corners = {
        Vector2{ rectangle.x, rectangle.y },
        Vector2{ rectangle.x + rectangle.width, rectangle.y },
        Vector2{ rectangle.x, rectangle.y + rectangle.height },
        Vector2{
            rectangle.x + rectangle.width,
            rectangle.y + rectangle.height
        }
    };
    for (Vector2 corner : corners) {
        minimumDistanceSquared = std::min(
            minimumDistanceSquared,
            PointToSegmentDistanceSquared(corner, start, end)
        );
    }

    float radiusSquared = radius * radius;
    return minimumDistanceSquared <= radiusSquared + GEOMETRY_EPSILON;
}
