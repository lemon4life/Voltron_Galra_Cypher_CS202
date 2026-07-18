#pragma once
#include "IAttackStrategy.h"

class RangedAttackStrategy : public IAttackStrategy {
private:
    Texture2D weaponTex;
    Texture2D muzzleFlashTex;
    Texture2D bulletTex;
    Vector2 recoilOffset;
    float muzzleFlashTimer;

public:
    RangedAttackStrategy(Texture2D tex, Texture2D muzzleTex, Texture2D bullTex);
    void Attack(Vector2 playerPos) override;
    void Update(float deltaTime) override;
    void Draw(Vector2 playerPos, bool facingLeft) override;
};
