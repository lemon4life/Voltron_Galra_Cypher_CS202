#pragma once
#include "Entities/Player/Paladin.h"
#include "Core/World/ObjectId.h"
#include "Entities/Projectile.h"
#include <vector>

struct ToxicParticle {
    Vector2 position;
    Vector2 velocity;
    float life;
    float maxLife;
    float frameTimer;
    int currentFrame;
    float scale;
    float alpha;
};

class Pidge : public Paladin {
private:
    float weaponRotation;
    bool isWeaponThrown;
    ObjectId thrownWeaponId = INVALID_OBJECT_ID;

    // Venom Zone State
    bool isVenomZoneActive = false;
    float venomZoneTimer = 0.0f;
    Vector2 venomZonePos = {0.0f, 0.0f};
    float toxicSpawnTimer = 0.0f;
    std::vector<ToxicParticle> toxicParticles;
    
    void UpdateVenomZone(float deltaTime);
    void DrawVenomZone() const;

public:
    Pidge(Vector2 startPos, CharacterSprites sprites);
    
    void Attack() override;
    void UseSkill() override;
    void UseUltimate() override;
    void ExecuteUltimateAction() override;
    
    bool IsWeaponVisible() const override { return !isWeaponThrown; }
    void CatchWeapon();
    
    void Draw() override;
    void DrawInactive() override;
    
    void Update(float deltaTime) override;
    void UpdateInactive(float deltaTime) override;
};
