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
    /// Creates a RangedAttackStrategy instance from the supplied configuration.
    RangedAttackStrategy(
        Texture2D tex,
        Texture2D muzzleTex,
        Texture2D bullTex,
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
