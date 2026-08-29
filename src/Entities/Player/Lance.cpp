#include "Entities/Player/Lance.h"
#include "Combat/RangedAttackStrategy.h"
#include "Entities/Player/PaladinDefinition.h"
#include "Combat/Buffs.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/UltimateIntroManager.h"
#include "Entities/Enemy.h"

Lance::Lance(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, PaladinCatalog::Get(PaladinId::Lance)),
      isUltimateFlash(false),
      ultimateFlashTimer(0.0f)
{
    introData = {"LANCE", "GLACIER PIERCE", BLUE, "Card_Lance", "lance_ult"};
    const WeaponDefinition& weapon =
        PaladinCatalog::Get(PaladinId::Lance).weapon;
    currentWeapon = std::make_unique<RangedAttackStrategy>(
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

bool Lance::IsWeaponVisible() const {
    return !HasPersonalBuff<DualWieldBuff>();
}

void Lance::Update(float deltaTime) {
    Paladin::Update(deltaTime);
    
    if (HasPersonalBuff<DualWieldBuff>()) {
        attackCooldown = baseAttackCooldown * 0.5f; // Halved attack cooldown
    } else {
        attackCooldown = baseAttackCooldown; // Normal / upgraded attack cooldown
    }
    
    if (ultimateFlashTimer > 0.0f) {
        ultimateFlashTimer -= deltaTime;
        if (ultimateFlashTimer <= 0.0f) {
            isUltimateFlash = false;
        }
    }

    UpdatePendingFreezeTargets(deltaTime);
}

void Lance::UpdateInactive(float deltaTime) {
    Paladin::UpdateInactive(deltaTime);
    UpdatePendingFreezeTargets(deltaTime);
}

void Lance::UpdatePendingFreezeTargets(float deltaTime) {
    ObjectManager& objects = GameManager::GetInstance().GetObjectManager();
    for (auto it = pendingFreezeTargets.begin();
         it != pendingFreezeTargets.end();) {
        it->delay -= deltaTime;
        if (it->delay > 0.0f) {
            ++it;
            continue;
        }

        Enemy* enemy = objects.FindEnemy(it->enemyId);
        if (enemy && !enemy->IsDead() && enemy->IsEnabled()) {
            AudioManager::GetInstance().PlaySoundEffect("fx_ice_hit");
            enemy->GetStatusComponent().AddEffect(
                EffectType::FREEZE,
                5.0f,
                0.0f
            );
        }
        it = pendingFreezeTargets.erase(it);
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
        Paladin::Draw();

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
    if (exEnergy < skillCost || isSkillActive) return;
    
    ActivateSkill(5.0f);
    AudioManager::GetInstance().PlaySoundEffect("fx_lance_skill");
    AudioManager::GetInstance().PlaySoundEffect("vl_lance_skill");
    AddPersonalBuff(std::make_unique<DualWieldBuff>(5.0f));
}

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
    
    Texture2D explodeTex = AssetManager::GetInstance().GetTexture("Ulti_explode");
    const std::vector<Enemy*>& enemies = GameManager::GetInstance()
        .GetObjectManager().GetEnemies();
    
    // 8-frame Ulti_explode over 0.48s (0.06s per frame).
    // Frame 0: 0.00s, Frame 1: 0.06s, Frame 2 (3rd frame): 0.12s
    float animDuration = 0.48f;
    float freezeDelay = (animDuration / 8.0f) * 2.0f;
    
    for (Enemy* enemy : enemies) {
        if (enemy && !enemy->IsDead() && enemy->IsEnabled()) {
            Vector2 enemyPos = enemy->GetPosition();
            if (explodeTex.id != 0) {
                GameManager::GetInstance().AddEffect(enemyPos, explodeTex, 8, animDuration, false);
            }
            pendingFreezeTargets.push_back({
                enemy->GetObjectId(),
                freezeDelay
            });
        }
    }
}
