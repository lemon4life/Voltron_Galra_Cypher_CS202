#include "Entities/EnemyEntities/EnemyDiver.h"

#include "AI/EnemyState.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Utils/LineOfSightGeometry.h"

#include "raymath.h"
#include "Core/Constants.h"

#include <algorithm>
#include <cmath>



namespace {
    constexpr int DIVER_MAX_HEALTH = 200;
    constexpr float DIVER_BASE_SPEED = 160.0f;
    constexpr int DIVER_DAMAGE = 70;
    constexpr float DIVER_ATTACK_COOLDOWN = 2.5f;
    constexpr float DIVER_SIGHT_DISTANCE = 40000.0f;
    constexpr float DIVER_OFF_SIGHT_DISTANCE = 40000.0f;
    constexpr float DIVE_STOP_DISTANCE = 85.0f;
    constexpr float DIVER_MIN_PLAYER_DISTANCE = 55.0f;
    constexpr float DIVER_LINE_OF_SIGHT_RADIUS = 3.0f;

    constexpr float READY_DURATION = 1.0f;
    constexpr float ATTACK_TELEGRAPH_REVEAL_DURATION = 0.7f;
    constexpr float READY_SPEED = 50.0f;
    constexpr float DIVE_SPEED = 300.0f; // DEBUG SLOW
    constexpr float DIVE_DURATION = 0.2f;
    constexpr float DIVE_RECOVERY_DURATION = 0.2f;
    constexpr float DIVER_KNOCKBACK_RESISTANCE = 0.50f;

    constexpr int DIVER_ATTACK_FRAME_COUNT = 4;
    constexpr float DIVER_ATTACK_FRAME_WIDTH = 96.0f;
    constexpr float DIVER_ATTACK_FRAME_HEIGHT = 32.0f;
    constexpr float DIVER_ATTACK_FRAME_DURATION = 0.05f;
    constexpr float DIVER_ATTACK_SCALE =
        Constants::GLOBAL_SCALE * 0.7f;
    // Both 96x32 images share pixel (0, 16) as their fixed world anchor.
    // The notification's visible area defines the promised attack lane.
    constexpr Vector2 DIVER_ATTACK_SPRITE_ANCHOR = { 0.0f, 16.0f };
    constexpr float DIVER_ATTACK_VISIBLE_END_X = 95.0f;
    constexpr float DIVER_ATTACK_VISIBLE_HEIGHT = 15.0f;
    constexpr float DIVER_ATTACK_REACH =
        DIVER_ATTACK_VISIBLE_END_X * DIVER_ATTACK_SCALE;
    constexpr float DIVER_ATTACK_RADIUS =
        DIVER_ATTACK_VISIBLE_HEIGHT * DIVER_ATTACK_SCALE * 0.5f;
    constexpr float DIVER_ATTACK_TELEGRAPH_OPACITY = 0.70f;

    constexpr Vector2 DIVER_SIZE = { 24.0f, 24.0f };
    constexpr Vector2 DIVER_RENDER_FOOT_OFFSET = { 0.0f, 16.0f };
    constexpr EnemyCollisionProfile DIVER_COLLISION_PROFILE = {
        { 18.0f, 8.0f },
        { 0.0f, 8.0f }
    };
}

/// Creates a EnemyDiver instance from the supplied configuration.
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
    SetRenderFootOffset(DIVER_RENDER_FOOT_OFFSET);
    SetCollisionProfile(DIVER_COLLISION_PROFILE);
    SetKnockbackResistance(DIVER_KNOCKBACK_RESISTANCE);

    SetEnemySprites(AssetManager::GetInstance().GetDiverSprites());
    attackNotification = AssetManager::GetInstance().GetTexture(
        "Diver_Attack_Notification"
    );
    ChangeState(GetIdleState());
}

/// Releases resources owned by this EnemyDiver instance.
EnemyDiver::~EnemyDiver() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

/// Advances this component's state for the current frame.
void EnemyDiver::Update(float deltaTime) {
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
    
    if (currentState) {
        currentState->Update(this, deltaTime);
    }
    kinematics.Update(deltaTime);
    
    if (health <= 0) return;

    UpdateMovementAnimationFlag(updateStartPosition);

    if (targetTeam && targetTeam->GetActivePaladin()) {
        bool useLockedAttackDirection =
            currentState == readyState.get() ||
            currentState == lungingState.get();
        Vector2 aimDirection = useLockedAttackDirection
            ? lockedAttackDirection
            : Vector2Subtract(
                targetTeam->GetActivePaladin()->GetPosition(),
                position
            );
        if (Vector2Length(aimDirection) > 0.0f) {
            aimDirection = Vector2Normalize(aimDirection);
        } else {
            aimDirection = { facingLeft ? -1.0f : 1.0f, 0.0f };
        }
        facingLeft = aimDirection.x < 0.0f;

        weaponAngle = atan2f(aimDirection.y, aimDirection.x) * RAD2DEG;
        if (facingLeft) weaponAngle += 180.0f; // Adjust because we flip the sprite
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
    
    if (playingEffect) {
        effectTimer += deltaTime;
        while (effectTimer >= DIVER_ATTACK_FRAME_DURATION) {
            effectTimer -= DIVER_ATTACK_FRAME_DURATION;
            currentEffectFrame++;
            if (currentEffectFrame >= DIVER_ATTACK_FRAME_COUNT) {
                currentEffectFrame = DIVER_ATTACK_FRAME_COUNT - 1;
                effectTimer = 0.0f;
                break;
            }
        }
    }
}

/// Renders this component using its current state and visual resources.
void EnemyDiver::Draw() {
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
        
        Vector2 offset = kinematics.GetOffset();
        Rectangle wDest = { position.x + offset.x, position.y + offset.y + 5.0f, (float)sprites.weapon.width, (float)sprites.weapon.height };
        Vector2 wOrigin = { 0.0f, sprites.weapon.height / 2.0f }; 
        if (facingLeft) {
            wOrigin.x = sprites.weapon.width;
        }
        
        DrawTexturePro(sprites.weapon, wSrc, wDest, wOrigin, weaponAngle, tint);
        
    }

    float attackAngle = std::atan2(
        lockedAttackDirection.y,
        lockedAttackDirection.x
    ) * RAD2DEG;
    if (attackTelegraphActive && attackNotification.id != 0) {
        float revealProgress = std::clamp(
            attackTelegraphElapsed / ATTACK_TELEGRAPH_REVEAL_DURATION,
            0.0f,
            1.0f
        );
        float revealedSourceWidth = DIVER_ATTACK_FRAME_WIDTH * revealProgress;
        if (revealedSourceWidth > 0.0f) {
            Rectangle source = {
                0.0f,
                0.0f,
                revealedSourceWidth,
                DIVER_ATTACK_FRAME_HEIGHT
            };
            Rectangle destination = {
                attackEffectStart.x,
                attackEffectStart.y,
                revealedSourceWidth * DIVER_ATTACK_SCALE,
                DIVER_ATTACK_FRAME_HEIGHT * DIVER_ATTACK_SCALE
            };
            DrawTexturePro(
                attackNotification,
                source,
                destination,
                {
                    DIVER_ATTACK_SPRITE_ANCHOR.x * DIVER_ATTACK_SCALE,
                    DIVER_ATTACK_SPRITE_ANCHOR.y * DIVER_ATTACK_SCALE
                },
                attackAngle,
                ColorAlpha(RED, DIVER_ATTACK_TELEGRAPH_OPACITY)
            );
        }
    }

    if (playingEffect && sprites.effect.id != 0) {
        Rectangle source = {
            currentEffectFrame * DIVER_ATTACK_FRAME_WIDTH,
            0.0f,
            DIVER_ATTACK_FRAME_WIDTH,
            DIVER_ATTACK_FRAME_HEIGHT
        };
        Rectangle destination = {
            attackEffectStart.x,
            attackEffectStart.y,
            DIVER_ATTACK_FRAME_WIDTH * DIVER_ATTACK_SCALE,
            DIVER_ATTACK_FRAME_HEIGHT * DIVER_ATTACK_SCALE
        };
        DrawTexturePro(
            sprites.effect,
            source,
            destination,
            {
                DIVER_ATTACK_SPRITE_ANCHOR.x * DIVER_ATTACK_SCALE,
                DIVER_ATTACK_SPRITE_ANCHOR.y * DIVER_ATTACK_SCALE
            },
            attackAngle,
            tint
        );

        if (Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES) {
            Vector2 attackEnd = GetAttackEffectEnd();
            DrawLineEx(
                attackEffectStart,
                attackEnd,
                DIVER_ATTACK_RADIUS * 2.0f,
                Fade(RED, 0.35f)
            );
            DrawCircleV(
                attackEffectStart,
                DIVER_ATTACK_RADIUS,
                Fade(RED, 0.35f)
            );
            DrawCircleV(
                attackEnd,
                DIVER_ATTACK_RADIUS,
                Fade(RED, 0.35f)
            );
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


/// Returns the current ready state.
EnemyDiverReadyState* EnemyDiver::GetReadyState() {
    return readyState.get();
}

/// Returns the current lunging state.
EnemyDiverLungingState* EnemyDiver::GetLungingState() {
    return lungingState.get();
}

/// Reports whether this component can perform enter ready state.
bool EnemyDiver::CanEnterReadyState() const {
    return attackCooldown <= 0.0f && IsWithinClearDiveRange();
}

/// Reports whether the within clear dive range condition is satisfied.
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

/// Reports whether the beyond disengage distance condition is satisfied.
bool EnemyDiver::IsBeyondDisengageDistance(Vector2 targetPosition) const {
    return Vector2Distance(position, targetPosition) > DIVER_OFF_SIGHT_DISTANCE;
}

/// Returns the current ready duration.
float EnemyDiver::GetReadyDuration() const {
    return READY_DURATION;
}

/// Returns the current ready speed.
float EnemyDiver::GetReadySpeed() const {
    return READY_SPEED;
}

/// Returns the current dive duration.
float EnemyDiver::GetDiveDuration() const {
    return DIVE_DURATION;
}

/// Returns the current dive speed.
float EnemyDiver::GetDiveSpeed() const {
    return DIVE_SPEED;
}

/// Returns the current dive stop distance.
float EnemyDiver::GetDiveStopDistance() const {
    return DIVE_STOP_DISTANCE;
}

/// Returns the current minimum player distance.
float EnemyDiver::GetMinimumPlayerDistance() const {
    return DIVER_MIN_PLAYER_DISTANCE;
}

/// Returns the current dive recovery duration.
float EnemyDiver::GetDiveRecoveryDuration() const {
    return DIVE_RECOVERY_DURATION;
}

/// Returns the current collision clearance radius.
float EnemyDiver::GetCollisionClearanceRadius() const {
    return DIVER_LINE_OF_SIGHT_RADIUS;
}

/// Calculates attack effect origin.
Vector2 EnemyDiver::CalculateAttackEffectOrigin() const {
    return position;
}

/// Returns the current attack effect end.
Vector2 EnemyDiver::GetAttackEffectEnd() const {
    return attackEffectEnd;
}

/// Begins attack preparation.
void EnemyDiver::BeginAttackPreparation(Vector2 direction) {
    if (Vector2Length(direction) > 0.0f) {
        lockedAttackDirection = Vector2Normalize(direction);
    } else {
        lockedAttackDirection = { facingLeft ? -1.0f : 1.0f, 0.0f };
    }
    // Pixel (0,16) of both effect sprites is fixed at the Diver's center
    // before the Diver starts backing up.
    // Charging and lunging movement must never move this warning or hitbox.
    attackEffectStart = CalculateAttackEffectOrigin();
    attackEffectEnd = {
        attackEffectStart.x +
            lockedAttackDirection.x * DIVER_ATTACK_REACH,
        attackEffectStart.y +
            lockedAttackDirection.y * DIVER_ATTACK_REACH
    };
    attackTelegraphElapsed = 0.0f;
    attackTelegraphActive = true;
    playingEffect = false;
}

/// Advances attack preparation.
void EnemyDiver::AdvanceAttackPreparation(float deltaTime) {
    attackTelegraphElapsed += std::max(0.0f, deltaTime);
}

/// Finishes attack preparation.
void EnemyDiver::EndAttackPreparation() {
    attackTelegraphActive = false;
}

/// Begins attack effect.
void EnemyDiver::BeginAttackEffect() {
    EndAttackPreparation();
    playingEffect = true;
    effectTimer = 0.0f;
    currentEffectFrame = 0;
    kinematics.ApplyThrust(lockedAttackDirection, DIVE_DURATION);
}

/// Finishes attack effect.
void EnemyDiver::EndAttackEffect() {
    playingEffect = false;
    effectTimer = 0.0f;
    currentEffectFrame = 0;
}

/// Implements the does attack hit behavior for this component.
bool EnemyDiver::DoesAttackHit(Rectangle targetBounds) const {
    if (!playingEffect) return false;
    return LineOfSightGeometry::CapsuleIntersectsRectangle(
        attackEffectStart,
        GetAttackEffectEnd(),
        DIVER_ATTACK_RADIUS,
        targetBounds
    );
}

/// Returns the current attack contact position.
Vector2 EnemyDiver::GetAttackContactPosition(
    Rectangle targetBounds
) const {
    Vector2 targetCenter = {
        targetBounds.x + targetBounds.width * 0.5f,
        targetBounds.y + targetBounds.height * 0.5f
    };
    Vector2 attackEnd = GetAttackEffectEnd();
    Vector2 attackSegment = Vector2Subtract(attackEnd, attackEffectStart);
    float segmentLengthSquared = Vector2LengthSqr(attackSegment);
    if (segmentLengthSquared <= 0.0f) return attackEffectStart;

    float projection = Vector2DotProduct(
        Vector2Subtract(targetCenter, attackEffectStart),
        attackSegment
    ) / segmentLengthSquared;
    projection = std::clamp(projection, 0.0f, 1.0f);
    return Vector2Add(
        attackEffectStart,
        Vector2Scale(attackSegment, projection)
    );
}
