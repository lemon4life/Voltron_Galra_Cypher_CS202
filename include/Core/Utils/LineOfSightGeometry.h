#pragma once

#include "raylib.h"

namespace LineOfSightGeometry {
    /// Tests the swept circular path against an axis-aligned blocking rectangle.
    bool CapsuleIntersectsRectangle(
        Vector2 start,
        Vector2 end,
        float radius,
        Rectangle rectangle
    );
}
