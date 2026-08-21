#pragma once

#include "Entities/Projectile.h"

class DemonTHABullet final : public Projectile {
private:
    void UpdateSweptCollisionBox(Vector2 previousPosition);

public:
    DemonTHABullet(
        Vector2 startPosition,
        Vector2 velocity,
        float lifetime,
        int damage,
        Texture2D texture
    );

    void Update(float deltaTime) override;
};
