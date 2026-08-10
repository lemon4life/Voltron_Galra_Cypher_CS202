#pragma once
#include "raylib.h"

class Paladin;

class IBuff {
public:
    virtual ~IBuff() = default;

    // Called once when the buff is first applied
    virtual void OnApply(Paladin* target) {}

    // Called every frame
    virtual void Update(float deltaTime, Paladin* activePaladin) = 0;

    // Called once when the buff expires or is removed
    virtual void OnRemove(Paladin* target) {}

    // Returns true if the buff has expired
    virtual bool IsFinished() const = 0;

    // Optional visual rendering
    virtual void Draw(Paladin* activePaladin) {}
};
