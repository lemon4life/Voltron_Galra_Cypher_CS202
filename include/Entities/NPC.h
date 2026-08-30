#pragma once
#include "Entities/GameObject.h"

enum class NpcId {
    Allura,
    Shiro
};

class NPC : public GameObject {
public:
    /// Creates a NPC instance from the supplied configuration.
    NPC(Vector2 pos, NpcId id = NpcId::Allura);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    /// Returns the current bounding box.
    Rectangle GetBoundingBox() const override;

    /// Returns the current npc id.
    NpcId GetNpcId() const { return npcId; }

private:
    NpcId npcId;
    int currentFrame;
    float frameTimer;
    float frameDuration;
    int numFrames;
};
