#include "Entities/EnemyEntities/EnemyChaser.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"

#include "raymath.h"
#include "Core/Constants.h"

#include <algorithm>

namespace {
    constexpr int CHASER_MAX_HEALTH = 80;
    constexpr float CHASER_SPEED = 150.0f;
    constexpr int CHASER_DAMAGE = 15;
    constexpr float CHASER_ATTACK_COOLDOWN = 1.f;
    constexpr float CHASER_SIGHT_DISTANCE = 40000.0f;
    constexpr float CHASER_AGGRO_RANGE = 40.0f;
    constexpr float CHASER_STOP_PATH_FINDING_DISTANCE = 30.0f;
    constexpr float CHASER_DAMAGE_CHARGE_DISTANCE = 64.0f;
    constexpr float CHASER_DAMAGE_CHARGE_DURATION = 0.25f;
    constexpr int CHASER_MIN_AGGRO_MILLISECONDS = 200;
    constexpr int CHASER_MAX_AGGRO_MILLISECONDS = 700;
    constexpr float CHASER_KNOCKBACK_RESISTANCE = 0.25f;
    constexpr Vector2 CHASER_SIZE = { 20.0f, 20.0f };
    constexpr Vector2 CHASER_RENDER_FOOT_OFFSET = { 0.0f, 16.0f };
    constexpr EnemyCollisionProfile CHASER_COLLISION_PROFILE = {
        { 16.0f, 8.0f },
        { 0.0f, 8.0f }
    };

    float RollAggroDuration() {
        return (float)GetRandomValue(
            CHASER_MIN_AGGRO_MILLISECONDS,
            CHASER_MAX_AGGRO_MILLISECONDS
        ) / 1000.0f;
    }
}

EnemyChaser::EnemyChaser(
    Vector2 position,
    TeamManager* targetTeam,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess
)
    : Enemy(
          position,
          targetTeam,
          CHASER_MAX_HEALTH,
          CHASER_SPEED,
          CHASER_DAMAGE,
          CHASER_ATTACK_COOLDOWN,
          removalAccess,
          pathAccess
      ) {
    idleState = std::make_unique<EnemyIdleState>(CHASER_SIGHT_DISTANCE);
    kinematics.SetType(WeaponKinematicsType::Melee);
    chaseState = std::make_unique<EnemyChaserChaseState>();
    damageState = std::make_unique<EnemyChaserDamageState>();
    enemyType = EnemyType::Chaser;
    size = CHASER_SIZE;
    SetRenderFootOffset(CHASER_RENDER_FOOT_OFFSET);
    SetCollisionProfile(CHASER_COLLISION_PROFILE);
    SetKnockbackResistance(CHASER_KNOCKBACK_RESISTANCE);
    ResetAggroMeter();

    SetEnemySprites(AssetManager::GetInstance().GetChaserSprites());
    ChangeState(GetIdleState());
}

EnemyChaser::~EnemyChaser() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void EnemyChaser::Update(float deltaTime) {
    Vector2 updateStartPosition = position;
    if (UpdateSpawnSequence(deltaTime)) {
        UpdateMovementAnimationFlag(updateStartPosition);
        return;
    }
    UpdateKnockback(deltaTime);
    
    if (statusComponent.Update(deltaTime, this)) {
        UpdateMovementAnimationFlag(updateStartPosition);
        return;
    }
    
    IEnemyState* lastState = currentState;
    if (currentState) {
        currentState->Update(this, deltaTime);
        kinematics.Update(deltaTime);
    }
    
    if (health <= 0) return;

    UpdateMovementAnimationFlag(updateStartPosition);

    if (targetTeam && targetTeam->GetActivePaladin()) {
        Vector2 targetPos = targetTeam->GetActivePaladin()->GetPosition();
        facingLeft = targetPos.x < position.x;
        
        Vector2 aimDir = Vector2Subtract(targetPos, position);
        weaponAngle = atan2f(aimDir.y, aimDir.x) * RAD2DEG;
        if (facingLeft) weaponAngle += 180.0f;
    }
    
    if (IsMovingForAnimation()) {
        runFrameTime += deltaTime;
        if (runFrameTime >= 0.08f) {
            currentRunFrame = (currentRunFrame + 1) % 8;
            runFrameTime = 0.0f;
        }
    } else {
        currentRunFrame = 0;
    }
    
    if (currentState == damageState.get() && lastState != currentState) {
        playingEffect = true;
        effectTimer = 0.0f;
        currentEffectFrame = 0;
        kinematics.ApplySwing(0.2f, 120.0f);
    }
    
    if (playingEffect) {
        effectTimer += deltaTime;
        if (effectTimer >= 0.05f) {
            currentEffectFrame++;
            effectTimer = 0.0f;
            if (currentEffectFrame >= 3) {
                playingEffect = false;
            }
        }
    }
}

void EnemyChaser::Draw() {
    if (!ShouldDrawDuringSpawn()) {
        DrawSpawnEffect();
        return;
    }

    Texture2D texToDraw = sprites.idle;
    if (health <= 0) {
        texToDraw = sprites.down;
    } else if (IsMovingForAnimation()) {
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
    
    Color tint = statusComponent.GetStatusTint();
    DrawTexturePro(texToDraw, src, dest, origin, 0.0f, tint);
    
    if (health > 0 && sprites.weapon.id != 0) {
        Rectangle wSrc = { 0, 0, (float)sprites.weapon.width, (float)sprites.weapon.height };
        if (facingLeft) wSrc.width = -wSrc.width;
        
        Rectangle wDest = { position.x, position.y + 5.0f, (float)sprites.weapon.width, (float)sprites.weapon.height };
        Vector2 wOrigin = { 0.0f, sprites.weapon.height / 2.0f }; 
        if (facingLeft) {
            wOrigin.x = sprites.weapon.width;
        }
        
        float currentAngle = weaponAngle + (facingLeft ? -kinematics.GetAngleOffset() : kinematics.GetAngleOffset());
        DrawTexturePro(sprites.weapon, wSrc, wDest, wOrigin, currentAngle, tint);
        
        if (playingEffect && sprites.effect.id != 0) {
            float efWidth = sprites.effect.width;
            efWidth /= 3.0f;
            Rectangle eSrc = { currentEffectFrame * efWidth, 0, efWidth, (float)sprites.effect.height };
            if (facingLeft) eSrc.width = -eSrc.width;
            
            float tipOffset = sprites.weapon.width * 0.8f;
            float radAngle = weaponAngle * DEG2RAD;
            if (facingLeft) radAngle += PI;
            Vector2 tipPos = {
                position.x + cosf(radAngle) * tipOffset,
                position.y + 5.0f + sinf(radAngle) * tipOffset
            };
            
            float scale = Constants::GLOBAL_SCALE;
            Rectangle eDest = { tipPos.x, tipPos.y, efWidth * scale, (float)sprites.effect.height * scale };
            Vector2 eOrigin = { 0.0f, (sprites.effect.height * scale) / 2.0f };
            if (facingLeft) eOrigin.x = efWidth * scale;
            
            DrawTexturePro(sprites.effect, eSrc, eDest, eOrigin, weaponAngle, tint);
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
    DrawSpawnEffect();
}


EnemyChaserDamageState* EnemyChaser::GetDamageState() {
    return damageState.get();
}

bool EnemyChaser::IsBeyondDisengageDistance(Vector2 targetPosition) const {
    return Vector2Distance(position, targetPosition) > CHASER_SIGHT_DISTANCE;
}

bool EnemyChaser::IsWithinAggroRange(Vector2 targetPosition) const {
    return Vector2Distance(position, targetPosition) <= CHASER_AGGRO_RANGE;
}

bool EnemyChaser::IsWithinStopPathFindingDistance(
    Vector2 targetPosition
) const {
    return Vector2Distance(position, targetPosition) <=
           CHASER_STOP_PATH_FINDING_DISTANCE;
}

float EnemyChaser::GetDamageChargeDistance() const {
    return CHASER_DAMAGE_CHARGE_DISTANCE;
}

float EnemyChaser::GetDamageChargeDuration() const {
    return CHASER_DAMAGE_CHARGE_DURATION;
}

void EnemyChaser::UpdateAggroMeter(
    bool isNearPlayer,
    float deltaTime
) {
    if (!isNearPlayer) {
        return;
    }

    aggroMeter = std::min(
        requiredAggroDuration,
        aggroMeter + std::max(0.0f, deltaTime)
    );
}

bool EnemyChaser::IsAggroReady() const {
    return aggroMeter >= requiredAggroDuration;
}

void EnemyChaser::ResetAggroMeter() {
    aggroMeter = 0.0f;
    requiredAggroDuration = RollAggroDuration();
}
