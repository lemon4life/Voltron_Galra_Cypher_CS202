#include "Entities/Player/Hunk.h"
#include "Combat/LaserAttackStrategy.h"
#include "Entities/Player/PaladinDefinition.h"

Hunk::Hunk(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, PaladinCatalog::Get(PaladinId::Hunk))
{
    const WeaponDefinition& weapon =
        PaladinCatalog::Get(PaladinId::Hunk).weapon;
    currentWeapon = new LaserAttackStrategy(
        sprites.weapon,
        sprites.muzzleFlash,
        sprites.bullet,
        sprites.impact,
        weapon.maximumDamage,
        weapon.recoil
    );
    if (currentWeapon) currentWeapon->SetOwner(this);
    texture = GetIdleTexture();
    
    // Base Stats Override
    maxHealth = 150;
    health = maxHealth;
    ghostHp = maxHealth;
    // Speed is determined by stats, wait I will override the default speed multiplier?
    // Hunk is slower, so let's set a slower max speed? But speed is not explicitly in Paladin class.
    // PaladinDefinition handles speed? I'll let definition handle it, or just set speed?
    // Let's check if Paladin has a speed variable.
    // Actually, I'll just leave speed alone since the plan says "speed is determined by PaladinDefinition", I can just not change it here if it's not accessible.
}

#include "Core/Manager/GameManager.h"
#include "Entities/Enemy.h"
#include "raymath.h"

void Hunk::ApplyKnockback(Vector2 dir, float force) {
    // Inherent Knockback Resistance: Hunk ignores collision forces!
}

void Hunk::Update(float deltaTime) {
    Paladin::Update(deltaTime);
    
    if (earthshatterFlashTimer > 0.0f) {
        earthshatterFlashTimer -= deltaTime;
        if (earthshatterFlashTimer <= 0.0f) {
            isEarthshatterFlash = false;
        }
    }
}

void Hunk::Draw() {
    Paladin::Draw();
    
    if (isEarthshatterFlash) {
        // Expand circle based on timer
        float progress = 1.0f - (earthshatterFlashTimer / 0.2f);
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;
        
        float currentRadius = knockbackRadius * progress;
        DrawCircleLines(position.x, position.y - 12.0f, currentRadius, ColorAlpha(BROWN, 0.6f * (1.0f - progress)));
    }
}

void Hunk::UseSkill() {
    if (!debugSpamMode && exEnergy < maxExEnergy / 3.0f) return;
    if (!debugSpamMode) exEnergy -= maxExEnergy / 3.0f;
    
    isEarthshatterFlash = true;
    earthshatterFlashTimer = 0.2f;
    
    const std::vector<GameObject*>& entities = GameManager::GetInstance().GetLevelEntities();
    for (GameObject* obj : entities) {
        Enemy* enemy = dynamic_cast<Enemy*>(obj);
        if (enemy && !enemy->IsDead()) {
            Vector2 ePos = enemy->GetPosition();
            Vector2 hPos = this->GetPosition();
            float dist = Vector2Distance(hPos, ePos);
            
            if (dist < knockbackRadius) {
                Vector2 dir = Vector2Subtract(ePos, hPos);
                if (Vector2Length(dir) > 0.0f) {
                    dir = Vector2Normalize(dir);
                    enemy->ApplyKnockback(dir, knockbackForce);
                }
                enemy->GetStatusComponent().AddEffect(EffectType::DIZZY, 2.0f);
            }
        }
    }
}

#include "Core/Manager/UltimateIntroManager.h"

void Hunk::UseUltimate() {
    if (!debugSpamMode && exEnergy < maxExEnergy) return;
    if (!debugSpamMode) exEnergy = 0.0f;
    
    UltimateIntroManager::GetInstance().PlayIntro(this);
}

void Hunk::ExecuteUltimateAction() {
    isInvulnerable = true;
    invulnerabilityTimer = 5.0f;
}
