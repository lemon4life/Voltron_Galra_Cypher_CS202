#pragma once

#include "Entities/GameObject.h"
#include "Entities/Player/PaladinDefinition.h"

class HubPaladinStand final : public GameObject {
private:
    PaladinId paladinId;
    Texture2D idleTexture;
    int currentFrame;
    float frameTimer;

public:
    /// Creates a HubPaladinStand instance from the supplied configuration.
    HubPaladinStand(
        PaladinId paladinId,
        Vector2 position,
        Texture2D idleTexture
    );

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    /// Returns the current bounding box.
    Rectangle GetBoundingBox() const override;

    /// Returns the current paladin id.
    PaladinId GetPaladinId() const { return paladinId; }
    /// Returns the current display name.
    const char* GetDisplayName() const;
    /// Reports whether the within interaction range condition is satisfied.
    bool IsWithinInteractionRange(Vector2 playerPosition) const;
};
