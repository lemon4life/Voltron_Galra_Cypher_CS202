#include "Entities/Player/Pidge.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/UltimateIntroManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/AssetManager.h"
#include "Combat/RangedAttackStrategy.h"
#include "Entities/Rover.h"
#include "Entities/Projectile.h"
#include "raymath.h"
#include <iostream>
#include <cstdlib>

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

void Pidge::UpdateVenomZone(float deltaTime) {
    if (!isVenomZoneActive) return;
    
    venomZoneTimer -= deltaTime;
    if (venomZoneTimer <= 0.0f) {
        isVenomZoneActive = false;
        toxicParticles.clear();
        return;
    }
    
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
    
    // Continuously spawn small animated toxic particles
    toxicSpawnTimer += deltaTime;
    if (toxicSpawnTimer >= 0.08f) {
        toxicSpawnTimer = 0.0f;
        
        float r = ((float)rand() / (float)RAND_MAX) * zoneRadius * 0.85f;
        float theta = ((float)rand() / (float)RAND_MAX) * 2.0f * PI;
        Vector2 spawnPos = {
            venomZonePos.x + r * cosf(theta),
            venomZonePos.y + r * sinf(theta)
        };
        
        Vector2 vel = {
            ((float)rand() / (float)RAND_MAX - 0.5f) * 20.0f,
            -15.0f - ((float)rand() / (float)RAND_MAX) * 20.0f
        };
        
        float life = 1.0f + ((float)rand() / (float)RAND_MAX) * 0.8f;
        float scale = 0.8f + ((float)rand() / (float)RAND_MAX) * 0.4f;
        
        toxicParticles.push_back({
            spawnPos, vel, life, life, 0.0f, 0, scale, 1.0f
        });
    }
    
    // Update active particles
    for (auto it = toxicParticles.begin(); it != toxicParticles.end(); ) {
        it->life -= deltaTime;
        if (it->life <= 0.0f) {
            it = toxicParticles.erase(it);
        } else {
            it->position.x += it->velocity.x * deltaTime;
            it->position.y += it->velocity.y * deltaTime;
            
            // 3-frame animation cycle
            it->frameTimer += deltaTime;
            if (it->frameTimer >= 0.12f) {
                it->frameTimer -= 0.12f;
                it->currentFrame = (it->currentFrame + 1) % 3;
            }
            
            float progress = it->life / it->maxLife;
            it->alpha = progress < 0.3f ? (progress / 0.3f) : 1.0f;
            
            ++it;
        }
    }
}

void Pidge::DrawVenomZone() const {
    if (!isVenomZoneActive) return;
    
    float zoneRadius = 120.0f;
    // Translucent poison zone fill
    DrawCircleV(venomZonePos, zoneRadius, ColorAlpha(DARKGREEN, 0.25f));
    
    // Red outline styling (matching reference visual)
    DrawCircleLines(venomZonePos.x, venomZonePos.y, zoneRadius, RED);
    DrawCircleLines(venomZonePos.x, venomZonePos.y, zoneRadius - 1.0f, ColorAlpha(RED, 0.7f));
    DrawCircleLines(venomZonePos.x, venomZonePos.y, zoneRadius + 1.0f, ColorAlpha(RED, 0.7f));
    
    // Draw animated toxic particles (3-frame snail-like sprites)
    Texture2D toxicTex = AssetManager::GetInstance().GetTexture("toxic");
    if (toxicTex.id != 0) {
        float frameWidth = (float)toxicTex.width / 3.0f;
        float frameHeight = (float)toxicTex.height;
        for (const auto& p : toxicParticles) {
            Rectangle src = { (float)p.currentFrame * frameWidth, 0.0f, frameWidth, frameHeight };
            Rectangle dest = { p.position.x, p.position.y, frameWidth * p.scale, frameHeight * p.scale };
            Vector2 origin = { frameWidth * p.scale * 0.5f, frameHeight * p.scale * 0.5f };
            DrawTexturePro(toxicTex, src, dest, origin, 0.0f, ColorAlpha(WHITE, p.alpha));
        }
    }
}

void Pidge::UpdateInactive(float deltaTime) {
    Paladin::UpdateInactive(deltaTime);
    UpdateVenomZone(deltaTime);
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
    
    UpdateVenomZone(deltaTime);
}

void Pidge::Attack() {
    if (isWeaponThrown) return; // Cannot attack while weapon is flying
    
    isWeaponThrown = true;

    // Fire Boomerang with buffed velocity and snappy reach
    Vector2 dir = currentAimVector;
    float speed = 1100.0f; // Fast, responsive projectile speed
    int baseDamage = BaseStats::Damage * PaladinCatalog::Get(PaladinId::Pidge).weapon.minDamageScalar;
    
    Projectile* projectile = SpawnLinearProjectile(
        dir, speed, baseDamage, 0.35f, true, sprites.weapon, true
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
    if (exEnergy < skillCost || isSkillActive) return;
    
    ActivateSkill(7.0f);
    AudioManager::GetInstance().PlaySoundEffect("fx_flash_lighting");
    
    isVenomZoneActive = true;
    venomZoneTimer = 7.0f;
    venomZonePos = position;
    toxicSpawnTimer = 0.0f;
    toxicParticles.clear();
    
    // Play the 8-frame skill_explode animation for the initial poison burst
    Texture2D explodeTex = AssetManager::GetInstance().GetTexture("skill_explode");
    if (explodeTex.id != 0) {
        GameManager::GetInstance().AddEffect(venomZonePos, explodeTex, 8, 0.5f, false);
    }
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
    DrawVenomZone();

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

void Pidge::DrawInactive() {
    DrawVenomZone();
    Paladin::DrawInactive();
}
