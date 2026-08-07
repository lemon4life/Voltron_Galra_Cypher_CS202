#pragma once

#include "Entities/GameObject.h"
#include "Core/DepthRenderItem.h"
#include <vector>

class DoorGate : public GameObject {
public:
    enum class State {
        OPEN,
        CLOSING,
        LOCKED,
        OPENING
    };

    DoorGate(Vector2 position);

    void Update(float deltaTime) override;
    void Draw() override;
    Rectangle GetBoundingBox() const override;

    void SetState(State newState);
    State GetState() const { return state; }
    
    // Support for two-pass rendering
    void DrawBaseLayer();
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items);

    bool IsSolid() const { return state != State::OPEN; }

private:
    State state;
    Texture2D tex;
    int currentFrame;
    float animationTimer;
    float frameDuration;
    int totalFrames;
};
