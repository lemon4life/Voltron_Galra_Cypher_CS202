#pragma once
#include "Entities/Player/Paladin.h"
#include "Entities/Projectile.h"

class Pidge : public Paladin {
private:
    float weaponRotation;
    bool isWeaponThrown;
    Projectile* thrownWeapon;

    // Venom Zone State
    bool isVenomZoneActive = false;
    float venomZoneTimer = 0.0f;
    Vector2 venomZonePos = {0.0f, 0.0f};
    
public:
    Pidge(Vector2 startPos, CharacterSprites sprites);
    
    void Attack() override;
    void UseSkill() override;
    void UseUltimate() override;
    void ExecuteUltimateAction() override;
    
    bool IsWeaponVisible() const override { return !isWeaponThrown; }
    void CatchWeapon();
    
    void Draw() override;
    
    void Update(float deltaTime) override;
    void UpdateInactive(float deltaTime) override;
};
