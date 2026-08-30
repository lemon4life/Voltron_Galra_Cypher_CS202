#pragma once
#include "Combat/IAttackStrategy.h"
#include "raylib.h"

class LaserAttackStrategy : public IAttackStrategy {
private:
    Texture2D weaponTex;
    Texture2D muzzleTex;
    Texture2D beamTex;
    Texture2D impactTex;

    float laserTimer;
    float maxLaserTime;
    
    Vector2 barrelTip;
    Vector2 laserEndPoint;
    Vector2 recoilOffset;
    float recoilStrength;
    int damage;

public:
    /// Creates a LaserAttackStrategy instance from the supplied configuration.
    LaserAttackStrategy(
        Texture2D weapon,
        Texture2D muzzle,
        Texture2D beam,
        Texture2D impact,
        int damage,
        float recoilStrength
    );
    /// Starts this attack behavior when its current conditions allow it.
    void Attack(Vector2 playerPos) override;
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw(Vector2 playerPos, bool facingLeft) override;
    /// Updates the stored damage.
    void SetDamage(int minDmg, int maxDmg) override { damage = maxDmg; }
};
