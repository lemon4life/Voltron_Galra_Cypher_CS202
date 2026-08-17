#pragma once

class IParticle {
public:
    virtual ~IParticle() = default;
    
    virtual void Update(float deltaTime) = 0;
    virtual void Draw() const = 0;
    virtual bool IsDead() const = 0;
};
