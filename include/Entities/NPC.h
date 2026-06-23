#pragma once
#include "Entities/GameObject.h"

class NPC : public GameObject {
public:
    NPC(Vector2 pos);
    void Update(float deltaTime) override;
    void Draw() override;
    Rectangle GetBoundingBox() const override;

private:
    int currentFrame;
    float frameTimer;
    float frameDuration;
    int numFrames;
};
