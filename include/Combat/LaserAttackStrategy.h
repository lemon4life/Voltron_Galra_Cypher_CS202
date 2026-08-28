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
    LaserAttackStrategy(
        Texture2D weapon,
        Texture2D muzzle,
        Texture2D beam,
        Texture2D impact,
        int damage,
        float recoilStrength
    );
    void Attack(Vector2 playerPos) override;
    void Update(float deltaTime) override;
    void Draw(Vector2 playerPos, bool facingLeft) override;
    void SetDamage(int minDmg, int maxDmg) override { damage = maxDmg; }
};
