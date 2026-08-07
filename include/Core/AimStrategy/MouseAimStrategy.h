#pragma once
#include "Core/AimStrategy/IAimStrategy.h"

class MouseAimStrategy : public IAimStrategy {
public:
    Vector2 CalculateAimVector(Paladin* paladin) override;
};
