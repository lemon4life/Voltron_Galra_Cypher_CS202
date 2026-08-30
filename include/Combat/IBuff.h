#pragma once
#include "raylib.h"

class Paladin;

// Design Pattern - Pluggable Component (not Decorator):
// Owners: Paladin and TeamManager. Component interface: IBuff. Concrete buffs:
// AegisShieldBuff, FireCircleBuff, and DualWieldBuff. They attach timed behavior
// but do not wrap or replace the Paladin interface like a true Decorator.
class IBuff {
public:
    /// Releases resources owned by this IBuff instance.
    virtual ~IBuff() = default;

    // Called once when the buff is first applied
    /// Handles the apply event.
    virtual void OnApply(Paladin* target) {}

    // Called every frame
    /// Advances this component's state for the current frame.
    virtual void Update(float deltaTime, Paladin* activePaladin) = 0;

    // Called once when the buff expires or is removed
    /// Handles the remove event.
    virtual void OnRemove(Paladin* target) {}

    // Returns true if the buff has expired
    /// Reports whether the finished condition is satisfied.
    virtual bool IsFinished() const = 0;

    // Optional visual rendering
    /// Renders this component using its current state and visual resources.
    virtual void Draw(Paladin* activePaladin) {}
};
