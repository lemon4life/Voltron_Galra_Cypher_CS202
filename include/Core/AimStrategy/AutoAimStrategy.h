#pragma once
#include "Core/AimStrategy/IAimStrategy.h"

class AutoAimStrategy : public IAimStrategy {
public:
    /// Calculates aim vector.
    Vector2 CalculateAimVector(Paladin* paladin) override;
};
