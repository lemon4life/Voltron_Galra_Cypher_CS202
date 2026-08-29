#pragma once

#include "Entities/Projectile.h"
#include <vector>

struct FireTrailNode {
    Vector2 position;
    float frameTimer;
    int currentFrame;
    float scale;
};

class KeithUltiProjectile final : public Projectile {
private:
    Vector2 startPosition;
    Vector2 travelDirection;
    float travelSpeed;
    float maxTravelTime;
    float trailDuration;
    float maxTrailDuration;
    float trailWidth;
    float currentTrailLength;
    float maxTrailLength;
    float lastSpawnDistance;
    float burnTickTimer;
    
    Texture2D fireAnimTexture;

    std::vector<FireTrailNode> fireNodes;

    void SpawnTrailNodes(float fromDist, float toDist);
    void UpdateTrail(float deltaTime);

public:
    KeithUltiProjectile(
        Vector2 startPos,
        Vector2 dir,
        float speed,
        int directDamage,
        float flightTime,
        float lingeringTrailTime,
        float width,
        Texture2D waveTexture,
        Texture2D fireTexture
    );

    void Update(float deltaTime) override;
    void Draw() override;
    bool IgnoresWorldCollision() const override { return true; }
};
