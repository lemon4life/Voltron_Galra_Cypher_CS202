#pragma once
#include "Core/AimStrategy/IAimStrategy.h"

class MouseAimStrategy : public IAimStrategy {
public:
    /// Calculates aim vector.
    Vector2 CalculateAimVector(Paladin* paladin) override;
};
