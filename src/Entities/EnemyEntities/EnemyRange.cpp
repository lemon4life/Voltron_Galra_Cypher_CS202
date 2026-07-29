#include "Entities/EnemyEntities/EnemyRange.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"

#include "AI/EnemyState.h"

#include "raymath.h"

namespace {
    constexpr int RANGE_MAX_HEALTH = 70;
    constexpr float RANGE_SPEED = 120.0f;
    constexpr int RANGE_DAMAGE = 12;
    constexpr float RANGE_ATTACK_COOLDOWN = 1.0f;
    constexpr float RANGE_DETECTION_DISTANCE = 700.0f;
    constexpr float RANGE_DISENGAGE_DISTANCE = 900.0f;
    constexpr float RANGE_SHOOTING_DISTANCE = 200.0f;
    constexpr float RANGE_PROJECTILE_SPEED = 320.0f;
    constexpr float RANGE_PROJECTILE_LIFETIME = 2.0f;
    constexpr float RANGE_PROJECTILE_RADIUS = 5.0f;
    constexpr float RANGE_MAX_PREDICTION_TIME = 1.0f;
    constexpr Vector2 RANGE_SIZE = { 24.0f, 24.0f };
}

EnemyRange::EnemyRange(
    Vector2 position,
    TeamManager* targetTeam,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess,
    ILevelLineOfSightQuery& lineOfSightQuery
)
    : Enemy(
          position,
          targetTeam,
          RANGE_MAX_HEALTH,
          RANGE_SPEED,
          RANGE_DAMAGE,
          RANGE_ATTACK_COOLDOWN,
          removalAccess,
          pathAccess
      ),
      lineOfSightQuery(lineOfSightQuery) {
    idleState = std::make_unique<EnemyIdleState>(RANGE_DETECTION_DISTANCE);
    chaseState = std::make_unique<EnemyRangeChaseState>();
    shootingState = std::make_unique<EnemyRangeShootingState>();
    enemyType = EnemyType::RANGE;
    kinematics.SetType(WeaponKinematicsType::Ranged);
    size = RANGE_SIZE;

    SetEnemySprites(AssetManager::GetInstance().GetRangeSprites());
    ChangeState(GetIdleState());
}

EnemyRange::~EnemyRange() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void EnemyRange::Update(float deltaTime) {
    UpdateKnockback(deltaTime);
    if (currentState) {
        currentState->Update(this, deltaTime);
    kinematics.Update(deltaTime);
    }
    
    if (health <= 0) return;

    if (targetTeam && targetTeam->GetActivePaladin()) {
        Vector2 targetPos = targetTeam->GetActivePaladin()->GetPosition();
        facingLeft = targetPos.x < position.x;
        
        Vector2 aimDir = Vector2Subtract(targetPos, position);
        weaponAngle = atan2f(aimDir.y, aimDir.x) * RAD2DEG;
        if (facingLeft) weaponAngle += 180.0f;
    }
    
    if (currentState == chaseState.get()) {
        runFrameTime += deltaTime;
        if (runFrameTime >= 0.08f) {
            currentRunFrame = (currentRunFrame + 1) % 8;
            runFrameTime = 0.0f;
        }
    } else {
        currentRunFrame = 0;
    }
}

void EnemyRange::Draw() {
    Texture2D texToDraw = sprites.idle;
    if (health <= 0) {
        texToDraw = sprites.down;
    } else if (currentState == chaseState.get()) {
        texToDraw = sprites.run;
    }

    float frameWidth = (float)texToDraw.width;
    if (texToDraw.id == sprites.run.id && sprites.run.id != 0) {
        frameWidth /= 8.0f;
    }
    float frameHeight = (float)texToDraw.height;
    
    Rectangle dest = { position.x, position.y, frameWidth, frameHeight };
    Vector2 origin = { frameWidth / 2.0f, frameHeight / 2.0f };
    
    Rectangle src = { 0, 0, frameWidth, frameHeight };
    if (texToDraw.id == sprites.run.id && sprites.run.id != 0) {
        src.x = currentRunFrame * frameWidth;
    }
    
    if (facingLeft) {
        src.width = -src.width;
    }
    
    DrawTexturePro(texToDraw, src, dest, origin, 0.0f, WHITE);
    
    if (health > 0 && sprites.weapon.id != 0) {
        Rectangle wSrc = { 0, 0, (float)sprites.weapon.width, (float)sprites.weapon.height };
        if (facingLeft) wSrc.width = -wSrc.width;
        
        Vector2 offset = kinematics.GetOffset();
        Rectangle wDest = { position.x + offset.x, position.y + offset.y + 5.0f, (float)sprites.weapon.width, (float)sprites.weapon.height };
        Vector2 wOrigin = { 0.0f, sprites.weapon.height / 2.0f }; 
        if (facingLeft) {
            wOrigin.x = sprites.weapon.width;
        }
        
        DrawTexturePro(sprites.weapon, wSrc, wDest, wOrigin, weaponAngle, WHITE);
        
        if (false) {
            float efWidth = sprites.effect.width;
            if (sprites.effect.width > 200) { // Heuristic: it's a spritesheet
                if (sprites.effect.id == AssetManager::GetInstance().GetTexture("Lance_Stab").id) efWidth /= 4.0f;
                else if (sprites.effect.id == AssetManager::GetInstance().GetTexture("Sword_Slash_Small").id) efWidth /= 3.0f;
            }
            Rectangle eSrc = { currentEffectFrame * efWidth, 0, efWidth, (float)sprites.effect.height };
            if (facingLeft) eSrc.width = -eSrc.width;
            
            float tipOffset = sprites.weapon.width * 0.8f;
            float radAngle = weaponAngle * DEG2RAD;
            if (facingLeft) radAngle += PI;
            Vector2 tipPos = {
                position.x + cosf(radAngle) * tipOffset,
                position.y + 5.0f + sinf(radAngle) * tipOffset
            };
            
            Rectangle eDest = { tipPos.x, tipPos.y, efWidth, (float)sprites.effect.height };
            Vector2 eOrigin = { 0.0f, sprites.effect.height / 2.0f };
            if (facingLeft) eOrigin.x = efWidth;
            
            DrawTexturePro(sprites.effect, eSrc, eDest, eOrigin, weaponAngle, WHITE);
        }
    }

    float healthPercent = (float)health / (float)maxHealth;
    DrawRectangle(
        (int)(position.x - size.x / 2.0f),
        (int)(position.y - frameHeight / 2.0f - 10.0f),
        (int)(size.x * healthPercent),
        4,
        RED
    );
}


EnemyRangeShootingState* EnemyRange::GetShootingState() {
    return shootingState.get();
}

bool EnemyRange::IsWithinShootingDistance(Vector2 targetPosition) const {
    return Vector2Distance(position, targetPosition) <= RANGE_SHOOTING_DISTANCE;
}

bool EnemyRange::IsBeyondDisengageDistance(Vector2 targetPosition) const {
    return Vector2Distance(position, targetPosition) > RANGE_DISENGAGE_DISTANCE;
}

float EnemyRange::GetProjectileSpeed() const {
    return RANGE_PROJECTILE_SPEED;
}

float EnemyRange::GetProjectileLifetime() const {
    return RANGE_PROJECTILE_LIFETIME;
}

float EnemyRange::GetProjectileRadius() const {
    return RANGE_PROJECTILE_RADIUS;
}

float EnemyRange::GetMaxPredictionTime() const {
    return RANGE_MAX_PREDICTION_TIME;
}
