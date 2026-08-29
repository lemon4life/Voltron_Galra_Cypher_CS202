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
    float remainingLifetime;
    float animationTimer = 0.0f;
    int animationFrame = 0;

    void UpdateCollisionBox();
    bool IsOutsideTravelBounds() const;

public:
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

    void Update(float deltaTime) override;
    void Draw() override;
    bool IgnoresWorldCollision() const override { return true; }
};
