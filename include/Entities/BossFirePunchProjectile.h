#pragma once

#include "Entities/Projectile.h"

class TeamManager;

class BossFirePunchProjectile final : public Projectile {
private:
    TeamManager* targetTeam;
    Texture2D animationTexture;
    Rectangle roomBounds;
    Rectangle mapBounds;
    float movementSpeed;
    float maximumTurnRateDegrees;
    float currentAngleDegrees;
    float animationTimer = 0.0f;
    int animationFrame = 0;

    /// Updates collision box.
    void UpdateCollisionBox();
    /// Reports whether the outside travel bounds condition is satisfied.
    bool IsOutsideTravelBounds() const;

public:
    /// Creates a BossFirePunchProjectile instance from the supplied configuration.
    BossFirePunchProjectile(
        Vector2 position,
        Vector2 initialTargetPosition,
        TeamManager* targetTeam,
        float speed,
        float maximumTurnRateDegrees,
        int damage,
        Texture2D texture,
        Rectangle roomBounds,
        Rectangle mapBounds
    );

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    /// Reports whether this projectile intentionally passes through world blockers.
    bool IgnoresWorldCollision() const override { return true; }
};
