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
    HubPaladinStand(
        PaladinId paladinId,
        Vector2 position,
        Texture2D idleTexture
    );

    void Update(float deltaTime) override;
    void Draw() override;
    Rectangle GetBoundingBox() const override;

    PaladinId GetPaladinId() const { return paladinId; }
    const char* GetDisplayName() const;
    bool IsWithinInteractionRange(Vector2 playerPosition) const;
};
