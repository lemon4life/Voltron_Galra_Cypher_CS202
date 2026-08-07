#pragma once
#include "Core/AimStrategy/IAimStrategy.h"

class AutoAimStrategy : public IAimStrategy {
public:
    Vector2 CalculateAimVector(Paladin* paladin) override;
};
