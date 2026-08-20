#include "Entities/Player/Keith.h"
#include "Core/Manager/AudioManager.h"
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
    currentWeapon = new MeleeAttackStrategy(
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
    if (exEnergy < skillCost) {
        return; 
    }
    
    if (teamManager) {
        teamManager->AddSharedBuff(std::make_unique<FireCircleBuff>(5.0f));
    }
    exEnergy -= skillCost;
    AudioManager::GetInstance().PlaySoundEffect("fx_fire");
}

void Keith::UseUltimate() {
    // Gate on Quintessence (shared team fuel) + individual cooldown
    if (ultimateCooldownTimer > 0.0f) return;
    if (!teamManager || !teamManager->ConsumeQuintessence(TeamManager::ULTIMATE_COST)) return;
    
    ultimateCooldownTimer = ULTIMATE_COOLDOWN_MAX;
    isUltimateAiming = true;
}

#include "Core/Manager/UltimateIntroManager.h"

void Keith::ExecuteUltimateAction() {
    AudioManager::GetInstance().PlaySoundEffect("fx_keith_ult");
    AudioManager::GetInstance().PlaySoundEffect("vl_keith_ult");
    const std::vector<Enemy*>& enemies = GameManager::GetInstance()
        .GetObjectManager().GetEnemies();
    float length = 300.0f;
    float width = 100.0f;
    
    for (Enemy* enemy : enemies) {
        if (enemy && !enemy->IsDead() && enemy->IsEnabled()) {
            Vector2 pivot = { position.x, position.y + 17.0f };
            Vector2 offset = Vector2Subtract(enemy->GetPosition(), pivot);
            
            // Rotate offset by -currentAimAngle
            float radAngle = -currentAimAngle;
            float cosA = cosf(radAngle);
            float sinA = sinf(radAngle);
            
            float localX = offset.x * cosA - offset.y * sinA;
            float localY = offset.x * sinA + offset.y * cosA;
            
            if (localX > 0 && localX < length && localY > -width/2.0f && localY < width/2.0f) {
                enemy->TakeDamage(100);
                enemy->GetStatusComponent().AddEffect(EffectType::BURN, 5.0f, 10.0f);
            }
        }
    }
    
    ultimateFlashTimer = 0.1f;
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
    
    if (ultimateFlashTimer > 0.0f) {
        ultimateFlashTimer -= deltaTime;
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
        float length = 300.0f;
        float width = 100.0f;
        Rectangle ghostRect = { position.x, position.y + 17.0f, length, width };
        Vector2 origin = { 0.0f, width / 2.0f };
        DrawRectanglePro(ghostRect, origin, currentAimAngle * RAD2DEG, ColorAlpha(ORANGE, 0.3f));
    }
    
    if (ultimateFlashTimer > 0.0f) {
        float length = 300.0f;
        float width = 100.0f;
        Rectangle flashRect = { position.x, position.y + 17.0f, length, width };
        Vector2 origin = { 0.0f, width / 2.0f };
        DrawRectanglePro(flashRect, origin, currentAimAngle * RAD2DEG, ColorAlpha(ORANGE, 0.8f));
    }
    
    Paladin::Draw();
}

void Keith::DrawInactive() {
    Paladin::DrawInactive();
}
