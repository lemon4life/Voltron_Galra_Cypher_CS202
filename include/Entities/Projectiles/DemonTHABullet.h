#pragma once

#include "Entities/Projectile.h"

class DemonTHABullet final : public Projectile {
private:
    /// Updates swept collision box.
    void UpdateSweptCollisionBox(Vector2 previousPosition);

public:
    /// Creates a DemonTHABullet instance from the supplied configuration.
    DemonTHABullet(
        Vector2 startPosition,
        Vector2 velocity,
        float lifetime,
        int damage,
        Texture2D texture
    );

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
};
