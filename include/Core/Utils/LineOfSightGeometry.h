#pragma once

#include "raylib.h"

namespace LineOfSightGeometry {
    bool CapsuleIntersectsRectangle(
        Vector2 start,
        Vector2 end,
        float radius,
        Rectangle rectangle
    );
}
