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

    /// Spawns trail nodes.
    void SpawnTrailNodes(float fromDist, float toDist);
    /// Updates trail.
    void UpdateTrail(float deltaTime);

public:
    /// Creates a KeithUltiProjectile instance from the supplied configuration.
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

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    /// Reports whether this projectile intentionally passes through world blockers.
    bool IgnoresWorldCollision() const override { return true; }
};
