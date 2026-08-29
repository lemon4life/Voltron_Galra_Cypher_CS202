#include "Entities/Player/Keith.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/UltimateIntroManager.h"
#include "Combat/MeleeAttackStrategy.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/AssetManager.h"
#include "Entities/Player/PaladinDefinition.h"
#include "Entities/Enemy.h"
#include "raymath.h"
#include "Combat/Buffs.h"

Keith::Keith(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, PaladinCatalog::Get(PaladinId::Keith))
{
    introData = {"KEITH", "EXCALIBUR", RED, "Card_Keith", "keith_ult_voice"};

    const WeaponDefinition& weapon =
        PaladinCatalog::Get(PaladinId::Keith).weapon;
    currentWeapon = std::make_unique<MeleeAttackStrategy>(
        sprites.weapon,
        AssetManager::GetInstance().GetTexture("Sword_Slash_Small"),
        AssetManager::GetInstance().GetTexture("Sword_Slash_Small"),
        BaseStats::Damage * weapon.minDamageScalar,
        BaseStats::Damage * weapon.maxDamageScalar
    );
    if (currentWeapon) currentWeapon->SetOwner(this);
    texture = GetIdleTexture();
    skillCost = maxExEnergy * 0.7f;
    hudPortraitSlice = { 228.0f, 59.0f, 194.0f, 84.0f };
}

void Keith::UseSkill() {
    if (exEnergy < skillCost || isSkillActive) {
        return; 
    }
    
    ActivateSkill(5.0f);
    if (teamManager) {
        teamManager->AddSharedBuff(std::make_unique<FireCircleBuff>(5.0f));
    }
    AudioManager::GetInstance().PlaySoundEffect("fx_fire");
}

void Keith::UseUltimate() {
    // Gate on Quintessence (shared team fuel) + individual cooldown
    if (ultimateCooldownTimer > 0.0f) return;
    if (!teamManager || !teamManager->ConsumeQuintessence(TeamManager::ULTIMATE_COST)) return;
    
    ultimateCooldownTimer = ULTIMATE_COOLDOWN_MAX;
    isUltimateAiming = true;
}

#include "Entities/Projectiles/KeithUltiProjectile.h"

void Keith::ExecuteUltimateAction() {
    AudioManager::GetInstance().PlaySoundEffect("fx_keith_ult");
    AudioManager::GetInstance().PlaySoundEffect("vl_keith_ult");
    
    Texture2D ultiFireTex = AssetManager::GetInstance().GetTexture("ulti_fire");
    Texture2D fireAnimTex = AssetManager::GetInstance().GetTexture("fire_anim");
    Vector2 dir = { cosf(currentAimAngle), sinf(currentAimAngle) };
    
    float speed = 750.0f;
    float flightTime = 0.65f; // Travels ~500px
    float lingeringTrailTime = 3.5f;
    float width = 70.0f;
    
    // Significantly scale up base and scalar damage per hit
    const PaladinDefinition& def = PaladinCatalog::Get(PaladinId::Keith);
    int damage = static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar * damageScalar * 3.5f);
    if (damage < 250) damage = 250;
    
    Vector2 spawnPos = GetWeaponPivot();
    auto projectile = std::make_unique<KeithUltiProjectile>(
        spawnPos,
        dir,
        speed,
        damage,
        flightTime,
        lingeringTrailTime,
        width,
        ultiFireTex,
        fireAnimTex
    );
    projectile->SetOwner(this);
    GameManager::GetInstance().AddProjectile(std::move(projectile));
}

// ProcessFireCircle is removed

void Keith::Update(float deltaTime) {
    Paladin::Update(deltaTime);
    
    if (skillCooldownTimer > 0.0f) {
        skillCooldownTimer -= deltaTime;
    }
    
    if (isUltimateAiming) {
        // Intercept Attack input to fire ultimate
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_J)) {
            isUltimateAiming = false;
            UltimateIntroManager::GetInstance().PlayIntro(this);
        }
    }
}

void Keith::UpdateInactive(float deltaTime) {
    Paladin::UpdateInactive(deltaTime);
    if (skillCooldownTimer > 0.0f) {
        skillCooldownTimer -= deltaTime;
    }
}

void Keith::Draw() {
    if (isUltimateAiming) {
        float length = 500.0f;
        float width = 70.0f;
        Vector2 pivot = GetWeaponPivot();
        Rectangle ghostRect = { pivot.x, pivot.y, length, width };
        Vector2 origin = { 0.0f, width * 0.5f };
        DrawRectanglePro(ghostRect, origin, currentAimAngle * RAD2DEG, ColorAlpha(ORANGE, 0.35f));
    }
    
    Paladin::Draw();
}

void Keith::DrawInactive() {
    Paladin::DrawInactive();
}
