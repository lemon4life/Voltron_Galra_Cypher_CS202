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
    
    /// Updates venom zone.
    void UpdateVenomZone(float deltaTime);
    /// Renders venom zone.
    void DrawVenomZone() const;

public:
    /// Creates a Pidge instance from the supplied configuration.
    Pidge(Vector2 startPos, CharacterSprites sprites);
    
    /// Starts this attack behavior when its current conditions allow it.
    void Attack() override;
    /// Activates skill.
    void UseSkill() override;
    /// Activates ultimate.
    void UseUltimate() override;
    /// Executes the gameplay effect after the Ultimate introduction finishes.
    void ExecuteUltimateAction() override;
    
    /// Reports whether the weapon visible condition is satisfied.
    bool IsWeaponVisible() const override { return !isWeaponThrown; }
    /// Handles catching weapon.
    void CatchWeapon();
    
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    /// Renders inactive.
    void DrawInactive() override;
    
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Updates inactive.
    void UpdateInactive(float deltaTime) override;
};
