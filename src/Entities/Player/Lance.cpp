#include "Entities/Player/Lance.h"
#include "Combat/RangedAttackStrategy.h"
#include "Entities/Player/PaladinDefinition.h"
#include "Combat/Buffs.h"
#include "Core/Manager/AudioManager.h"

Lance::Lance(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, PaladinCatalog::Get(PaladinId::Lance)),
      isUltimateFlash(false),
      ultimateFlashTimer(0.0f)
{
    introData = {"LANCE", "GLACIER PIERCE", BLUE, "Card_Lance", "lance_ult"};
    const WeaponDefinition& weapon =
        PaladinCatalog::Get(PaladinId::Lance).weapon;
    currentWeapon = new RangedAttackStrategy(
        sprites.weapon,
        sprites.muzzleFlash,
        sprites.bullet,
        BaseStats::Damage * weapon.maxDamageScalar,
        weapon.recoil
    );
    if (currentWeapon) currentWeapon->SetOwner(this);
    texture = GetIdleTexture();
    skillCost = maxExEnergy * 0.5f;
    hudPortraitSlice = { 92.0f, 71.0f, 194.0f, 84.0f };
}

void Lance::Update(float deltaTime) {
    Paladin::Update(deltaTime);
    
    if (HasPersonalBuff<DualWieldBuff>()) {
        attackCooldown = 0.1f; // Halved attack cooldown
    } else {
        attackCooldown = 0.2f; // Normal attack cooldown
    }
    
    if (ultimateFlashTimer > 0.0f) {
        ultimateFlashTimer -= deltaTime;
        if (ultimateFlashTimer <= 0.0f) {
            isUltimateFlash = false;
        }
    }
}

void Lance::Attack() {
    if (!HasPersonalBuff<DualWieldBuff>()) {
        Paladin::Attack();
    } else {
        if (currentWeapon) {
            Vector2 normal = { -currentAimVector.y, currentAimVector.x };
            // Flatten the vertical orbit so they align more horizontally when aiming left/right
            normal.y *= 0.2f;
            Vector2 pivot = GetWeaponPivot();
            
            Vector2 leftPivot = { pivot.x + normal.x * 6.0f, pivot.y + normal.y * 12.0f };
            Vector2 rightPivot = { pivot.x - normal.x * 6.0f, pivot.y - normal.y * 12.0f };
            
            currentWeapon->Attack(leftPivot);
            currentWeapon->Attack(rightPivot);
        }
    }
}

void Lance::Draw() {
    if (!HasPersonalBuff<DualWieldBuff>()) {
        Paladin::Draw();
    } else {
        // Suppress drawing the single weapon from Paladin::Draw by temporarily unsetting it
        IAttackStrategy* tempWeapon = currentWeapon;
        currentWeapon = nullptr;
        Paladin::Draw();
        currentWeapon = tempWeapon;
        
        if (currentWeapon) {
            Vector2 normal = { -currentAimVector.y, currentAimVector.x };
            // Flatten the vertical orbit so they align more horizontally when aiming left/right
            normal.y *= 0.2f;
            Vector2 pivot = GetWeaponPivot();
            
            Vector2 leftPivot = { pivot.x + normal.x * 6.0f, pivot.y + normal.y * 12.0f };
            Vector2 rightPivot = { pivot.x - normal.x * 6.0f, pivot.y - normal.y * 12.0f };
            
            currentWeapon->Draw(leftPivot, facingLeft);
            currentWeapon->Draw(rightPivot, facingLeft);
        }
    }
    
    if (isUltimateFlash) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(SKYBLUE, 0.4f));
    }
}

void Lance::UseSkill() {
    if (exEnergy < skillCost) return;
    
    exEnergy -= skillCost;
    AudioManager::GetInstance().PlaySoundEffect("fx_lance_skill");
    AudioManager::GetInstance().PlaySoundEffect("vl_lance_skill");
    AddPersonalBuff(std::make_unique<DualWieldBuff>(5.0f));
}

#include "Core/Manager/GameManager.h"
#include "Entities/Enemy.h"

#include "Core/Manager/UltimateIntroManager.h"
#include "Core/Manager/TeamManager.h"

void Lance::UseUltimate() {
    // Gate on Quintessence (shared team fuel) + individual cooldown
    if (ultimateCooldownTimer > 0.0f) return;
    if (!teamManager || !teamManager->ConsumeQuintessence(TeamManager::ULTIMATE_COST)) return;
    
    ultimateCooldownTimer = ULTIMATE_COOLDOWN_MAX;
    AudioManager::GetInstance().PlaySoundEffect("vl_lance_ult");
    UltimateIntroManager::GetInstance().PlayIntro(this);
}

void Lance::ExecuteUltimateAction() {
    isUltimateFlash = true;
    ultimateFlashTimer = 0.2f; // Short flash
    AudioManager::GetInstance().PlaySoundEffect("fx_lance_ult");
    AudioManager::GetInstance().PlaySoundEffect("fx_ice_explode");
    
    const std::vector<GameObject*>& entities = GameManager::GetInstance().GetLevelEntities();
    for (GameObject* obj : entities) {
        Enemy* enemy = dynamic_cast<Enemy*>(obj);
        if (enemy && !enemy->IsDead() && enemy->IsEnabled()) {
            enemy->GetStatusComponent().AddEffect(EffectType::FREEZE, 5.0f, 0.0f);
        }
    }
}
