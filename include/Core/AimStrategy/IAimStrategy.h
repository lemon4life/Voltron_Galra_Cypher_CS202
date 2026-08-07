#pragma once
#include "raylib.h"

class Paladin;

class IAimStrategy {
public:
    virtual ~IAimStrategy() = default;
    
    // Calculates and returns the normalized aiming vector
    virtual Vector2 CalculateAimVector(Paladin* paladin) = 0;
};
