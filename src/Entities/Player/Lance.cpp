#include "Entities/Player/Lance.h"
#include "Combat/RangedAttackStrategy.h"
#include "Entities/Player/PaladinDefinition.h"

Lance::Lance(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, PaladinCatalog::Get(PaladinId::Lance)),
      isDualWielding(false),
      dualWieldTimer(0.0f),
      isUltimateFlash(false),
      ultimateFlashTimer(0.0f)
{
    const WeaponDefinition& weapon =
        PaladinCatalog::Get(PaladinId::Lance).weapon;
    currentWeapon = new RangedAttackStrategy(
        sprites.weapon,
        sprites.muzzleFlash,
        sprites.bullet,
        weapon.maximumDamage,
        weapon.recoil
    );
    if (currentWeapon) currentWeapon->SetOwner(this);
    texture = GetIdleTexture();
}

void Lance::Update(float deltaTime) {
    Paladin::Update(deltaTime);
    
    if (isDualWielding) {
        dualWieldTimer -= deltaTime;
        attackCooldown = 0.2f; // Halved attack cooldown
        if (dualWieldTimer <= 0.0f) {
            isDualWielding = false;
        }
    } else {
        attackCooldown = 0.4f; // Normal attack cooldown
    }
    
    if (ultimateFlashTimer > 0.0f) {
        ultimateFlashTimer -= deltaTime;
        if (ultimateFlashTimer <= 0.0f) {
            isUltimateFlash = false;
        }
    }
}

void Lance::Attack() {
    if (!isDualWielding) {
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
    if (!isDualWielding) {
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
    if (!debugSpamMode && exEnergy < maxExEnergy / 3.0f) return;
    
    if (!debugSpamMode) exEnergy -= maxExEnergy / 3.0f;
    isDualWielding = true;
    dualWieldTimer = 5.0f;
}

#include "Core/Manager/GameManager.h"
#include "Entities/Enemy.h"

void Lance::UseUltimate() {
    if (!debugSpamMode && exEnergy < maxExEnergy) return;
    
    if (!debugSpamMode) exEnergy = 0.0f;
    isUltimateFlash = true;
    ultimateFlashTimer = 0.2f; // Short flash
    
    const std::vector<GameObject*>& entities = GameManager::GetInstance().GetLevelEntities();
    for (GameObject* obj : entities) {
        Enemy* enemy = dynamic_cast<Enemy*>(obj);
        if (enemy && !enemy->IsDead()) {
            enemy->GetStatusComponent().AddEffect(EffectType::FREEZE, 5.0f, 0.0f);
        }
    }
}
