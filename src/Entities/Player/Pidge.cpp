#include "Entities/Player/Pidge.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/UltimateIntroManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/AudioManager.h"
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
    thrownWeaponId = INVALID_OBJECT_ID;
    
    const WeaponDefinition& weapon = PaladinCatalog::Get(PaladinId::Pidge).weapon;
    currentWeapon = new RangedAttackStrategy(
        sprites.weapon,
        sprites.muzzleFlash,
        sprites.bullet,
        BaseStats::Damage * weapon.maxDamageScalar,
        weapon.recoil
    );
    if (currentWeapon) currentWeapon->SetOwner(this);
    skillCost = maxExEnergy * 0.7f;
    hudPortraitSlice = { 144.0f, 96.0f, 194.0f, 84.0f };
}

void Pidge::UpdateInactive(float deltaTime) {
    Paladin::UpdateInactive(deltaTime);
    if (isVenomZoneActive) {
        venomZoneTimer -= deltaTime;
        if (venomZoneTimer <= 0.0f) {
            isVenomZoneActive = false;
        } else {
            float zoneRadius = 120.0f;
            const auto& enemies = GameManager::GetInstance()
                .GetObjectManager().GetEnemies();
            for (Enemy* enemy : enemies) {
                    if (!enemy || enemy->IsDead() || !enemy->IsEnabled()) continue;
                    
                    if (CheckCollisionCircles(venomZonePos, zoneRadius, enemy->GetPosition(), 16.0f)) {
                        enemy->GetStatusComponent().AddEffect(EffectType::POISON, 1.1f, 5.0f);
                        enemy->GetStatusComponent().AddEffect(EffectType::SLOW, 1.1f);
                    }
            }
        }
    }
}

void Pidge::Update(float deltaTime) {
    Paladin::Update(deltaTime);
    
    // Check if thrown weapon is no longer active (e.g. destroyed by something else or expired)
    Projectile* thrownWeapon = dynamic_cast<Projectile*>(
        GameManager::GetInstance().GetObjectManager().FindObject(
            thrownWeaponId
        )
    );
    if (isWeaponThrown && (!thrownWeapon || !thrownWeapon->IsActive())) {
        CatchWeapon();
    }
    
    // Venom Zone Logic
    if (isVenomZoneActive) {
        venomZoneTimer -= deltaTime;
        if (venomZoneTimer <= 0.0f) {
            isVenomZoneActive = false;
        } else {
            float zoneRadius = 120.0f;
            const auto& enemies = GameManager::GetInstance()
                .GetObjectManager().GetEnemies();
            for (Enemy* enemy : enemies) {
                    if (!enemy || enemy->IsDead() || !enemy->IsEnabled()) continue;
                    
                    if (CheckCollisionCircles(venomZonePos, zoneRadius, enemy->GetPosition(), 16.0f)) {
                        enemy->GetStatusComponent().AddEffect(EffectType::POISON, 1.1f, 5.0f);
                        enemy->GetStatusComponent().AddEffect(EffectType::SLOW, 1.1f);
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
    int baseDamage = BaseStats::Damage * PaladinCatalog::Get(PaladinId::Pidge).weapon.minDamageScalar;
    
    Projectile* projectile = SpawnLinearProjectile(
        dir, speed, baseDamage, 0.5f, true, sprites.weapon, true
    );
    thrownWeaponId = projectile
        ? projectile->GetObjectId()
        : INVALID_OBJECT_ID;
    if (!projectile) isWeaponThrown = false;
}

void Pidge::CatchWeapon() {
    isWeaponThrown = false;
    thrownWeaponId = INVALID_OBJECT_ID;
}

void Pidge::UseSkill() {
    if (exEnergy < skillCost) return;
    exEnergy -= skillCost;
    
    AudioManager::GetInstance().PlaySoundEffect("fx_flash_lighting");
    
    isVenomZoneActive = true;
    venomZoneTimer = 7.0f;
    venomZonePos = position;
}

void Pidge::UseUltimate() {
    // Gate on Quintessence (shared team fuel) + individual cooldown
    if (ultimateCooldownTimer > 0.0f) return;
    if (!teamManager || !teamManager->ConsumeQuintessence(TeamManager::ULTIMATE_COST)) return;
    ultimateCooldownTimer = ULTIMATE_COOLDOWN_MAX;
    AudioManager::GetInstance().PlaySoundEffect("fx_pidge_ult");
    AudioManager::GetInstance().PlaySoundEffect("vl_pidge_ult");
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
    Projectile* thrownWeapon = dynamic_cast<Projectile*>(
        GameManager::GetInstance().GetObjectManager().FindObject(
            thrownWeaponId
        )
    );
    if (isWeaponThrown && thrownWeapon && thrownWeapon->IsActive()) {
        DrawLineEx(GetWeaponPivot(), thrownWeapon->GetPosition(), 2.0f, GREEN);
    }
    Paladin::Draw();
}
