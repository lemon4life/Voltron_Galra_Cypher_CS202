#pragma once

class IGameState {
public:
    virtual ~IGameState() = default;
    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;
};
