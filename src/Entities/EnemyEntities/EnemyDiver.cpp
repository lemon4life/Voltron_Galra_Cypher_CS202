#include "Entities/EnemyEntities/EnemyDiver.h"

#include "AI/EnemyState.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/AssetManager.h"

#include "raymath.h"
#include "Core/Constants.h"

#include <algorithm>



namespace {
    constexpr int DIVER_MAX_HEALTH = 200;
    constexpr float DIVER_BASE_SPEED = 160.0f;
    constexpr int DIVER_DAMAGE = 70;
    constexpr float DIVER_ATTACK_COOLDOWN = 2.5f;
    constexpr float DIVER_SIGHT_DISTANCE = 40000.0f;
    constexpr float DIVER_OFF_SIGHT_DISTANCE = 40000.0f;
    constexpr float DIVE_STOP_DISTANCE = 70.f;

    constexpr float READY_DURATION = 0.3f;
    constexpr float READY_SPEED = 50.0f;
    constexpr float DIVE_SPEED = 300.0f; // DEBUG SLOW
    constexpr float DIVE_DURATION = 0.2f;
    constexpr float DIVE_RECOVERY_DURATION = 0.2f;
    constexpr float DIVER_KNOCKBACK_RESISTANCE = 0.50f;

    constexpr Vector2 DIVER_SIZE = { 24.0f, 24.0f };
    constexpr EnemyCollisionProfile DIVER_COLLISION_PROFILE = {
        { 18.0f, 8.0f },
        { 0.0f, 8.0f }
    };
}

EnemyDiver::EnemyDiver(
    Vector2 position,
    TeamManager* targetTeam,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess,
    ILevelLineOfSightQuery& lineOfSightQuery
)
    : Enemy(
          position,
          targetTeam,
          DIVER_MAX_HEALTH,
          DIVER_BASE_SPEED,
          DIVER_DAMAGE,
          DIVER_ATTACK_COOLDOWN,
          removalAccess,
          pathAccess
      ),
      lineOfSightQuery(lineOfSightQuery) {
    idleState = std::make_unique<EnemyIdleState>(DIVER_SIGHT_DISTANCE);
    kinematics.SetType(WeaponKinematicsType::Thrust);
    chaseState = std::make_unique<EnemyDiverChaseState>();
    readyState = std::make_unique<EnemyDiverReadyState>();
    lungingState = std::make_unique<EnemyDiverLungingState>();
    enemyType = EnemyType::DIVER;
    size = DIVER_SIZE;
    SetCollisionProfile(DIVER_COLLISION_PROFILE);
    SetKnockbackResistance(DIVER_KNOCKBACK_RESISTANCE);

    SetEnemySprites(AssetManager::GetInstance().GetDiverSprites());
    ChangeState(GetIdleState());
}

EnemyDiver::~EnemyDiver() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void EnemyDiver::Update(float deltaTime) {
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
        if (facingLeft) weaponAngle += 180.0f; // Adjust because we flip the sprite
    }
    
    if (currentState == chaseState.get() || currentState == lungingState.get() || currentState == readyState.get()) {
        runFrameTime += deltaTime;
        if (runFrameTime >= 0.08f) {
            currentRunFrame = (currentRunFrame + 1) % 8;
            runFrameTime = 0.0f;
        }
    } else {
        currentRunFrame = 0;
    }
    
    if (currentState == lungingState.get()) {
        if (!playingEffect) {
            playingEffect = true;
            effectTimer = 0.0f;
            currentEffectFrame = 0;
            Vector2 thrustDir = { cosf(weaponAngle * DEG2RAD), sinf(weaponAngle * DEG2RAD) };
            if (facingLeft) thrustDir = { -thrustDir.x, -thrustDir.y };
            kinematics.ApplyThrust(thrustDir, 0.2f);
            
            float tipOffset = sprites.weapon.width * 0.8f;
            float radAngle = weaponAngle * DEG2RAD;
            if (facingLeft) radAngle += PI;
            staticEffectPos = {
                position.x + cosf(radAngle) * tipOffset,
                position.y + 5.0f + sinf(radAngle) * tipOffset
            };
        }
    } else {
        playingEffect = false;
    }
    
    if (playingEffect) {
        effectTimer += deltaTime;
        if (effectTimer >= 0.05f) {
            currentEffectFrame++;
            effectTimer = 0.0f;
            if (currentEffectFrame >= 4) {
                currentEffectFrame = 3;
            }
        }
    }
}

void EnemyDiver::Draw() {
    Texture2D texToDraw = sprites.idle;
    if (health <= 0) {
        texToDraw = sprites.down;
    } else if (currentState == chaseState.get() || currentState == lungingState.get() || currentState == readyState.get()) {
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
        
        if (playingEffect && sprites.effect.id != 0) {
            float efWidth = sprites.effect.width;
            efWidth /= 4.0f;
            Rectangle eSrc = { currentEffectFrame * efWidth, 0, efWidth, (float)sprites.effect.height };
            if (facingLeft) eSrc.width = -eSrc.width;
            
            Vector2 tipPos = staticEffectPos;
            
            float scale = Constants::GLOBAL_SCALE;
            Rectangle eDest = { tipPos.x, tipPos.y, efWidth * scale, (float)sprites.effect.height * scale };
            Vector2 eOrigin = { 0.0f, (sprites.effect.height * scale) / 2.0f };
            if (facingLeft) eOrigin.x = efWidth * scale;
            
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


EnemyDiverReadyState* EnemyDiver::GetReadyState() {
    return readyState.get();
}

EnemyDiverLungingState* EnemyDiver::GetLungingState() {
    return lungingState.get();
}

bool EnemyDiver::CanEnterReadyState() const {
    return attackCooldown <= 0.0f && IsWithinClearDiveRange();
}

bool EnemyDiver::IsWithinClearDiveRange() const {
    Paladin* target = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    if (!target) return false;

    if (Vector2Distance(position, target->GetPosition()) > DIVE_STOP_DISTANCE) {
        return false;
    }

    return lineOfSightQuery.HasClearLineOfSight(
        position,
        target->GetPosition(),
        GetCollisionClearanceRadius()
    );
}

bool EnemyDiver::IsBeyondDisengageDistance(Vector2 targetPosition) const {
    return Vector2Distance(position, targetPosition) > DIVER_OFF_SIGHT_DISTANCE;
}

float EnemyDiver::GetReadyDuration() const {
    return READY_DURATION;
}

float EnemyDiver::GetReadySpeed() const {
    return READY_SPEED;
}

float EnemyDiver::GetDiveDuration() const {
    return DIVE_DURATION;
}

float EnemyDiver::GetDiveSpeed() const {
    return DIVE_SPEED;
}

float EnemyDiver::GetDiveStopDistance() const {
    return DIVE_STOP_DISTANCE;
}

float EnemyDiver::GetDiveRecoveryDuration() const {
    return DIVE_RECOVERY_DURATION;
}

float EnemyDiver::GetCollisionClearanceRadius() const {
    return std::max(size.x, size.y) / 2.0f;
}
