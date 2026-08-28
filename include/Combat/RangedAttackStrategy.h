#pragma once
#include "IAttackStrategy.h"
#include "Combat/WeaponKinematics.h"

class RangedAttackStrategy : public IAttackStrategy {
private:
    Texture2D weaponTex;
    Texture2D muzzleFlashTex;
    Texture2D bulletTex;
    WeaponKinematics kinematics;
    float muzzleFlashTimer;
    int damage;
    float recoilStrength;

public:
    RangedAttackStrategy(
        Texture2D tex,
        Texture2D muzzleTex,
        Texture2D bullTex,
        int damage,
        float recoilStrength
    );
    void Attack(Vector2 playerPos) override;
    void Update(float deltaTime) override;
    void Draw(Vector2 playerPos, bool facingLeft) override;
    void SetDamage(int minDmg, int maxDmg) override { damage = maxDmg; }
};
