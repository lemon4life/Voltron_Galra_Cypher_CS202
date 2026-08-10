#include "Entities/Player/Pidge.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/UltimateIntroManager.h"
#include "Combat/RangedAttackStrategy.h"
#include "Entities/Rover.h"
#include "Entities/Projectile.h"
#include "raymath.h"
#include <iostream>

Pidge::Pidge(Vector2 startPos, CharacterSprites sprites)
    : Paladin(startPos, sprites, PaladinCatalog::Get(PaladinId::Pidge)) {
    introData = {"PIDGE", "ROVER OVERRIDE", GREEN, "Card_Pidge", "pidge_ult_voice"};
    weaponRotation = 0.0f;
    isWeaponThrown = false;
    thrownWeapon = nullptr;
    
    // Setup Boomerang as RangedAttackStrategy for now, or just leave it empty and override Attack
    // Wait, the plan was to use RangedAttackStrategy with isReturning inside Projectile.
    const WeaponDefinition& weapon = PaladinCatalog::Get(PaladinId::Pidge).weapon;
    currentWeapon = new RangedAttackStrategy(
        sprites.weapon,
        sprites.muzzleFlash,
        sprites.bullet,
        weapon.maximumDamage,
        weapon.recoil
    );
    if (currentWeapon) currentWeapon->SetOwner(this);
}

void Pidge::Update(float deltaTime) {
    Paladin::Update(deltaTime);
    
    // Check if thrown weapon is no longer active (e.g. destroyed by something else or expired)
    if (isWeaponThrown && thrownWeapon && !thrownWeapon->IsActive()) {
        CatchWeapon();
    }
    
    // Venom Zone Logic
    if (isVenomZoneActive) {
        venomZoneTimer -= deltaTime;
        if (venomZoneTimer <= 0.0f) {
            isVenomZoneActive = false;
        } else {
            float zoneRadius = 120.0f;
            const auto& entities = GameManager::GetInstance().GetLevelEntities();
            for (const auto& e : entities) {
                if (e->GetObjectType() == GameObjectType::Enemy) {
                    Enemy* enemy = static_cast<Enemy*>(e);
                    if (!enemy || enemy->IsDead()) continue;
                    
                    if (CheckCollisionCircles(venomZonePos, zoneRadius, enemy->GetPosition(), 16.0f)) {
                        enemy->GetStatusComponent().AddEffect(EffectType::POISON, 1.1f, 5.0f);
                        enemy->GetStatusComponent().AddEffect(EffectType::SLOW, 1.1f);
                    }
                }
            }
        }
    }
}

void Pidge::Attack() {
    if (isWeaponThrown) return; // Cannot attack while weapon is flying
    
    isWeaponThrown = true;

    // Fire Boomerang
    Vector2 dir = currentAimVector;
    float speed = 800.0f; // Fast Boomerang speed
    Vector2 vel = Vector2Scale(dir, speed);
    
    int baseDamage = PaladinCatalog::Get(PaladinId::Pidge).weapon.minimumDamage;
    
    Projectile* proj = new Projectile(GetWeaponPivot(), vel, 5.0f, baseDamage, sprites.weapon, false);
    proj->SetReturning(false);
    proj->SetPiercing(true);
    proj->SetMaxFlyTime(0.35f); // Slightly faster max fly time
    proj->SetOwner(this);
    
    float rot = atan2(dir.y, dir.x) * (180.0f / PI);
    proj->SetFixedRotation(true, rot);
    
    thrownWeapon = proj;
    GameManager::GetInstance().AddProjectile(proj);
}

void Pidge::CatchWeapon() {
    isWeaponThrown = false;
    thrownWeapon = nullptr;
}

void Pidge::UseSkill() {
    bool debugSpamMode = true;
    float skillExCost = 30.0f; // example cost
    if (!debugSpamMode && exEnergy < skillExCost) return;
    
    if (!debugSpamMode) exEnergy -= skillExCost;
    
    isVenomZoneActive = true;
    venomZoneTimer = 7.0f;
    venomZonePos = position;
}

void Pidge::UseUltimate() {
    bool debugSpamMode = true;
    if (!debugSpamMode && exEnergy < maxExEnergy) return;
    if (!debugSpamMode) exEnergy = 0.0f;
    
    UltimateIntroManager::GetInstance().PlayIntro(this);
}

void Pidge::ExecuteUltimateAction() {
    auto rover = std::make_unique<Rover>(position, this, GetTeamManager());
    GameManager::GetInstance().AddRover(std::move(rover));
}

void Pidge::Draw() {
    // Draw Venom Zone
    if (isVenomZoneActive) {
        DrawCircleV(venomZonePos, 120.0f, ColorAlpha(GREEN, 0.4f));
    }

    // Draw visual tether if weapon is thrown
    if (isWeaponThrown && thrownWeapon && thrownWeapon->IsActive()) {
        DrawLineEx(GetWeaponPivot(), thrownWeapon->GetPosition(), 2.0f, GREEN);
    }
    Paladin::Draw();
}
