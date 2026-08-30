#pragma once

class IParticle {
public:
    /// Releases resources owned by this IParticle instance.
    virtual ~IParticle() = default;
    
    /// Advances this component's state for the current frame.
    virtual void Update(float deltaTime) = 0;
    /// Renders this component using its current state and visual resources.
    virtual void Draw() const = 0;
    /// Reports whether the dead condition is satisfied.
    virtual bool IsDead() const = 0;
};
