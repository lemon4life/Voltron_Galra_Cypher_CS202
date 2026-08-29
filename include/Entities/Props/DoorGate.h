#pragma once

#include "Core/World/MapObject.h"
#include <vector>

class DoorGate : public MapObject {
public:
    enum class State {
        OPEN,
        CLOSING,
        LOCKED,
        OPENING
    };

    DoorGate(Vector2 position);

    void Update(float deltaTime) override;
    Rectangle GetBoundingBox() const override;
    Rectangle GetCollisionBox() const override;

    void SetState(State newState);
    State GetState() const { return state; }
    void SetProjectileBarrierActive(bool active) {
        projectileBarrierActive = active;
    }
    bool BlocksProjectiles() const { return projectileBarrierActive; }
    
    // Support for two-pass rendering
    void DrawBaseLayer() override;
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items) override;

    bool IsSolid() const override { return state != State::OPEN; }

private:
    State state;
    Texture2D tex;
    int currentFrame;
    float animationTimer;
    float frameDuration;
    int totalFrames;
    bool projectileBarrierActive;
};
