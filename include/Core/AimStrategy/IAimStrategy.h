#pragma once
#include "raylib.h"

class Paladin;

// Design Pattern - Strategy:
// Context: Paladin, selected by TeamManager. Strategy interface: IAimStrategy.
// Concrete strategies: AutoAimStrategy and MouseAimStrategy. Switching the
// strategy changes target calculation without changing Paladin movement code.
class IAimStrategy {
public:
    /// Releases resources owned by this IAimStrategy instance.
    virtual ~IAimStrategy() = default;
    
    // Calculates and returns the normalized aiming vector
    /// Calculates aim vector.
    virtual Vector2 CalculateAimVector(Paladin* paladin) = 0;
};
