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

    /// Creates a DoorGate instance from the supplied configuration.
    DoorGate(Vector2 position);

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Returns the current bounding box.
    Rectangle GetBoundingBox() const override;
    /// Returns the current collision box.
    Rectangle GetCollisionBox() const override;

    /// Updates the stored state.
    void SetState(State newState);
    /// Returns the current state.
    State GetState() const { return state; }
    /// Updates the stored projectile barrier active.
    void SetProjectileBarrierActive(bool active) {
        projectileBarrierActive = active;
    }
    /// Implements the blocks projectiles behavior for this component.
    bool BlocksProjectiles() const { return projectileBarrierActive; }
    
    // Support for two-pass rendering
    /// Renders base layer.
    void DrawBaseLayer() override;
    /// Adds depth render items.
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items) override;

    /// Reports whether the solid condition is satisfied.
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
