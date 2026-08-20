#pragma once
#include "Entities/GameObject.h"

enum class NpcId {
    Allura,
    Shiro
};

class NPC : public GameObject {
public:
    NPC(Vector2 pos, NpcId id = NpcId::Allura);
    void Update(float deltaTime) override;
    void Draw() override;
    Rectangle GetBoundingBox() const override;

    NpcId GetNpcId() const { return npcId; }

private:
    NpcId npcId;
    int currentFrame;
    float frameTimer;
    float frameDuration;
    int numFrames;
};
