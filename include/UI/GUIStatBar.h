#pragma once

#include "raylib.h"

#include <string>

class GUIStatBar {
public:
    static void Draw(
        Rectangle bounds,
        const std::string& label,
        const std::string& exactValue,
        float value,
        float comparisonMaximum,
        Color fillColor,
        bool enabled = true
    );
};
