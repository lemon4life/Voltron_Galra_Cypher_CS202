#include "Entities/Player/Keith.h"
#include "Combat/MeleeAttackStrategy.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/AssetManager.h"
#include "Entities/Player/PaladinDefinition.h"
#include "Entities/Enemy.h"
#include "raymath.h"

Keith::Keith(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, PaladinCatalog::Get(PaladinId::Keith))
{
    const WeaponDefinition& weapon =
        PaladinCatalog::Get(PaladinId::Keith).weapon;
    currentWeapon = new MeleeAttackStrategy(
        sprites.weapon,
        AssetManager::GetInstance().GetTexture("Sword_Slash_Small"),
        AssetManager::GetInstance().GetTexture("Sword_Slash_Small"),
        weapon.minimumDamage,
        weapon.maximumDamage
    );
    if (currentWeapon) currentWeapon->SetOwner(this);
    texture = GetIdleTexture();
}

void Keith::UseSkill() {
    if (!debugSpamMode && skillCooldownTimer > 0.0f) {
        return; // Skill on cooldown
    }
    
    isFireCircleActive = true;
    fireCircleTimer = 5.0f;
    
    if (!debugSpamMode) {
        skillCooldownTimer = SKILL_COOLDOWN;
    }
}

void Keith::UseUltimate() {
    if (!debugSpamMode && exEnergy < maxExEnergy) {
        return; // Not enough EX
    }
    
    isUltimateAiming = true;
    
    if (!debugSpamMode) {
        exEnergy = 0.0f; // Reset EX meter after casting
    }
}

#include "Core/Manager/UltimateIntroManager.h"

void Keith::ExecuteUltimateAction() {
    float length = 300.0f;
    float width = 100.0f;
    
    const std::vector<GameObject*>& entities = GameManager::GetInstance().GetLevelEntities();
    for (GameObject* obj : entities) {
        Enemy* enemy = dynamic_cast<Enemy*>(obj);
        if (enemy && !enemy->IsDead()) {
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

void Keith::ProcessFireCircle(float deltaTime, Vector2 centerPos) {
    if (!isFireCircleActive) return;
    
    fireCircleTimer -= deltaTime;
    if (fireCircleTimer <= 0.0f) {
        isFireCircleActive = false;
    } else {
        float skillRadius = 100.0f;
        const std::vector<GameObject*>& entities = GameManager::GetInstance().GetLevelEntities();
        for (GameObject* obj : entities) {
            Enemy* enemy = dynamic_cast<Enemy*>(obj);
            if (enemy && !enemy->IsDead()) {
                if (CheckCollisionCircles(centerPos, skillRadius, enemy->GetPosition(), 15.0f)) {
                    enemy->GetStatusComponent().AddEffect(EffectType::BURN, 5.0f, 5.0f);
                    OnHitEnemy(5); // Constant EX generation on Fire Circle hit
                }
            }
        }
    }
}

void Keith::Update(float deltaTime) {
    Paladin::Update(deltaTime);
    
    if (skillCooldownTimer > 0.0f) {
        skillCooldownTimer -= deltaTime;
    }
    
    if (isFireCircleActive) {
        ProcessFireCircle(deltaTime, position);
        fireCirclePos = position;
    }
    
    if (isUltimateAiming) {
        // Intercept Attack input to fire ultimate
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_J)) {
            isUltimateAiming = false;
            UltimateIntroManager::GetInstance().PlayIntro(this);
        }
    }
    
    ProcessFireCircle(deltaTime, position);
    fireCirclePos = position; // Track for drawing
    
    if (ultimateFlashTimer > 0.0f) {
        ultimateFlashTimer -= deltaTime;
    }
}

void Keith::UpdateInactive(float deltaTime) {
    if (skillCooldownTimer > 0.0f) {
        skillCooldownTimer -= deltaTime;
    }
    
    if (isFireCircleActive) {
        if (TeamManager* tm = GetTeamManager()) {
            Paladin* active = tm->GetActivePaladin();
            if (active) {
                ProcessFireCircle(deltaTime, active->GetPosition());
                fireCirclePos = active->GetPosition();
            }
        }
    }
}

void Keith::Draw() {
    if (isFireCircleActive) {
        DrawCircleV(fireCirclePos, 100.0f, ColorAlpha(RED, 0.3f));
    }
    
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
    if (isFireCircleActive) {
        DrawCircleV(fireCirclePos, 100.0f, ColorAlpha(RED, 0.3f));
    }
}
