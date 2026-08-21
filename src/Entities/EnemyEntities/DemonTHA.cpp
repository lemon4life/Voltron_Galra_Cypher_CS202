#include "Entities/EnemyEntities/DemonTHA.h"

#include "AI/DemonTHAState.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Projectile.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr int DEMON_MAX_HEALTH = 200;
    constexpr float DEMON_SPEED = 70.0f;
    constexpr int DEMON_DAMAGE = 15;
    constexpr float DEMON_ATTACK_COOLDOWN = 0.4f;
    constexpr float DEMON_KNOCKBACK_RESISTANCE = 0.25f;

    constexpr float BODY_FRAME_WIDTH = 64.0f;
    constexpr float BODY_FRAME_HEIGHT = 64.0f;
    constexpr int IDLE_FRAME_COUNT = 4;
    constexpr int RUN_FRAME_COUNT = 5;
    constexpr int SHOOTING_FRAME_COUNT = 4;
    constexpr float IDLE_FRAME_DURATION = 0.15f;
    constexpr float RUN_FRAME_DURATION = 0.10f;
    constexpr float SHOOTING_FRAME_DURATION = 0.10f;
    constexpr Vector2 BODY_DRAW_ORIGIN = { 30.5f, 32.5f };
    constexpr Vector2 BODY_GUN_ROOT = { 33.0f, 28.0f };
    constexpr Vector2 DEMON_SIZE = { 34.0f, 30.0f };
    constexpr Vector2 DEMON_RENDER_FOOT_OFFSET = { 0.0f, 14.5f };
    constexpr EnemyCollisionProfile DEMON_COLLISION_PROFILE = {
        { 14.0f, 8.0f },
        { 0.0f, 10.5f }
    };

    constexpr float GUN_FRAME_WIDTH = 35.0f;
    constexpr float GUN_FRAME_HEIGHT = 17.0f;
    constexpr int GUN_SHOOTING_FRAME_COUNT = 4;
    constexpr int GUN_FIRE_FRAME_INDEX = 1;
    constexpr float GUN_FRAME_DURATION = 0.10f;
    constexpr Vector2 GUN_ROOT = { 19.0f, 0.0f };
    constexpr Vector2 GUN_MUZZLE_POINT = { 2.0f, 10.0f };
    constexpr Vector2 GUN_REAR_ALIGNMENT_POINT = { 29.0f, 10.0f };
    constexpr bool SOURCE_ART_FACES_LEFT = true;

    constexpr float AGGRO_DISTANCE = 100.0f;
    constexpr int DISTANT_IDLE_AGGRO_PERCENT = 30;
    constexpr float PROJECTILE_SPEED = 320.0f;
    constexpr float PROJECTILE_LIFETIME = 2.0f;
    constexpr float PROJECTILE_RADIUS = 5.0f;
    constexpr float HEALTH_BAR_GAP = 8.0f;
    constexpr float HEALTH_BAR_HEIGHT = 4.0f;
    constexpr float MIN_DIRECTION_LENGTH = 0.001f;

    Vector2 RotateVector(Vector2 value, float angleDegrees) {
        float angle = angleDegrees * DEG2RAD;
        float cosine = std::cos(angle);
        float sine = std::sin(angle);
        return {
            value.x * cosine - value.y * sine,
            value.x * sine + value.y * cosine
        };
    }

    Vector2 GetPlayerCenter(const Paladin& player) {
        Rectangle bounds = player.GetBoundingBox();
        return {
            bounds.x + bounds.width * 0.5f,
            bounds.y + bounds.height * 0.5f
        };
    }
}

DemonTHA::DemonTHA(
    Vector2 position,
    TeamManager* targetTeam,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess,
    ILevelLineOfSightQuery& lineOfSightQuery
)
    : Enemy(
          position,
          targetTeam,
          DEMON_MAX_HEALTH,
          DEMON_SPEED,
          DEMON_DAMAGE,
          DEMON_ATTACK_COOLDOWN,
          removalAccess,
          pathAccess
      ),
      lineOfSightQuery(lineOfSightQuery) {
    enemyType = EnemyType::DEMON_THA;
    size = DEMON_SIZE;
    SetRenderFootOffset(DEMON_RENDER_FOOT_OFFSET);
    SetCollisionProfile(DEMON_COLLISION_PROFILE);
    SetKnockbackResistance(DEMON_KNOCKBACK_RESISTANCE);
    SetAttackCooldown(0.0f);

    wanderIdleState = std::make_unique<DemonTHAWanderIdleState>();
    wanderMoveState = std::make_unique<DemonTHAWanderMoveState>();
    aggroState = std::make_unique<DemonTHAAggroState>();

    AssetManager& assets = AssetManager::GetInstance();
    idleTexture = assets.GetTexture("THA_Idle");
    runTexture = assets.GetTexture("THA_Run");
    shootingTexture = assets.GetTexture("THA_Shooting");
    idleGunTexture = assets.GetTexture("THA_Gun_Idle");
    shootingGunTexture = assets.GetTexture("THA_Gun_Shooting");
    SetEnemySprites({
        idleTexture,
        runTexture,
        { 0 },
        idleGunTexture,
        { 0 },
        assets.GetTexture("Knight_Gun_Bullet")
    });

    ChangeState(wanderIdleState.get());
}

DemonTHA::~DemonTHA() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

bool DemonTHA::IsAggroing() const {
    return currentState == aggroState.get();
}

Paladin* DemonTHA::GetActiveTarget() const {
    return targetTeam ? targetTeam->GetActivePaladin() : nullptr;
}

DemonTHA::GunPose DemonTHA::CalculateGunPose(
    Vector2 entityPosition,
    Vector2 targetPosition
) const {
    bool desiredFacingLeft = targetPosition.x < entityPosition.x;
    bool flipSprite = desiredFacingLeft != SOURCE_ART_FACES_LEFT;

    float bodyOriginX = flipSprite
        ? BODY_FRAME_WIDTH - 1.0f - BODY_DRAW_ORIGIN.x
        : BODY_DRAW_ORIGIN.x;
    float bodyRootX = flipSprite
        ? BODY_FRAME_WIDTH - 1.0f - BODY_GUN_ROOT.x
        : BODY_GUN_ROOT.x;
    Vector2 anchor = {
        entityPosition.x - bodyOriginX + bodyRootX,
        entityPosition.y - BODY_DRAW_ORIGIN.y + BODY_GUN_ROOT.y
    };

    float gunRootX = flipSprite
        ? GUN_FRAME_WIDTH - 1.0f - GUN_ROOT.x
        : GUN_ROOT.x;
    float muzzleX = flipSprite
        ? GUN_FRAME_WIDTH - 1.0f - GUN_MUZZLE_POINT.x
        : GUN_MUZZLE_POINT.x;
    float rearAlignmentX = flipSprite
        ? GUN_FRAME_WIDTH - 1.0f - GUN_REAR_ALIGNMENT_POINT.x
        : GUN_REAR_ALIGNMENT_POINT.x;
    Vector2 muzzleFromRoot = {
        muzzleX - gunRootX,
        GUN_MUZZLE_POINT.y - GUN_ROOT.y
    };
    Vector2 barrelForward = Vector2Normalize({
        muzzleX - rearAlignmentX,
        GUN_MUZZLE_POINT.y - GUN_REAR_ALIGNMENT_POINT.y
    });
    Vector2 barrelNormal = {
        -barrelForward.y,
        barrelForward.x
    };
    Vector2 targetDirection = Vector2Subtract(targetPosition, anchor);
    float targetDistance = Vector2Length(targetDirection);
    float targetAngle = std::atan2(
        targetDirection.y,
        targetDirection.x
    ) * RAD2DEG;
    float sourceForwardAngle = std::atan2(
        barrelForward.y,
        barrelForward.x
    ) * RAD2DEG;
    float desiredForwardAngle = targetAngle;
    float signedBarrelOffset = Vector2DotProduct(
        muzzleFromRoot,
        barrelNormal
    );
    if (targetDistance > std::fabs(signedBarrelOffset) + 0.001f) {
        float alignmentRatio = std::clamp(
            signedBarrelOffset / targetDistance,
            -1.0f,
            1.0f
        );
        desiredForwardAngle -= std::asin(alignmentRatio) * RAD2DEG;
    }
    float angleDegrees = desiredForwardAngle - sourceForwardAngle;
    Vector2 muzzleOffset = RotateVector(muzzleFromRoot, angleDegrees);

    return {
        anchor,
        { gunRootX, GUN_ROOT.y },
        Vector2Add(anchor, muzzleOffset),
        angleDegrees,
        flipSprite
    };
}

bool DemonTHA::HasClearShotFrom(
    Vector2 entityPosition,
    const Paladin& target
) const {
    Vector2 targetPosition = GetPlayerCenter(target);
    GunPose pose = CalculateGunPose(entityPosition, targetPosition);
    return lineOfSightQuery.HasClearLineOfSight(
               entityPosition,
               targetPosition,
               PROJECTILE_RADIUS
           ) &&
           lineOfSightQuery.HasClearLineOfSight(
               entityPosition,
               pose.muzzleWorld,
               PROJECTILE_RADIUS
           ) &&
           lineOfSightQuery.HasClearLineOfSight(
               pose.muzzleWorld,
               targetPosition,
               PROJECTILE_RADIUS
           );
}

bool DemonTHA::ShouldImmediatelyAggro() const {
    Paladin* target = GetActiveTarget();
    return target &&
        Vector2Distance(position, GetPlayerCenter(*target)) <=
            AGGRO_DISTANCE &&
        HasClearShotFrom(position, *target);
}

bool DemonTHA::ShouldRollDistantAggroOnIdleEntry() const {
    Paladin* target = GetActiveTarget();
    if (!target ||
        Vector2Distance(position, GetPlayerCenter(*target)) <=
            AGGRO_DISTANCE ||
        !HasClearShotFrom(position, *target)) {
        return false;
    }
    return GetRandomValue(1, 100) <= DISTANT_IDLE_AGGRO_PERCENT;
}

bool DemonTHA::ConsumeNextWanderGoalUsesLineOfSight() {
    bool useLineOfSight = nextWanderGoalUsesLineOfSight;
    nextWanderGoalUsesLineOfSight = !nextWanderGoalUsesLineOfSight;
    return useLineOfSight;
}

float DemonTHA::GetCurrentRoomCandidateRadius() const {
    Rectangle levelBounds = pathAccess.GetLevelBounds();
    return std::max(
        1.0f,
        std::hypot(levelBounds.width, levelBounds.height)
    );
}

void DemonTHA::SetGunShooting(bool shooting) {
    if (gunShooting == shooting) return;
    gunShooting = shooting;
    gunFrameTimer = 0.0f;
    gunFrameIndex = 0;
}

DemonTHA::BodyAnimation DemonTHA::GetDesiredBodyAnimation() const {
    if (IsAggroing()) return BodyAnimation::Shooting;
    return IsMovingForAnimation()
        ? BodyAnimation::Run
        : BodyAnimation::Idle;
}

bool DemonTHA::TryFireAtActivePlayer() {
    Paladin* target = GetActiveTarget();
    if (!target || GetAttackCooldown() > 0.0f ||
        !HasClearShotFrom(position, *target)) {
        return false;
    }

    Vector2 playerCenter = GetPlayerCenter(*target);
    GunPose pose = CalculateGunPose(position, playerCenter);
    Vector2 direction = Vector2Subtract(
        playerCenter,
        pose.muzzleWorld
    );
    if (Vector2Length(direction) <= MIN_DIRECTION_LENGTH) return false;
    direction = Vector2Normalize(direction);

    Texture2D projectileTexture =
        AssetManager::GetInstance().GetTexture("Knight_Gun_Bullet");
    auto* projectile = new Projectile(
        pose.muzzleWorld,
        Vector2Scale(direction, PROJECTILE_SPEED),
        PROJECTILE_LIFETIME,
        GetDamage(),
        projectileTexture,
        true,
        PROJECTILE_RADIUS
    );
    GameManager::GetInstance().AddProjectile(projectile);
    AudioManager::GetInstance().PlayRandomLaser();
    ResetAttackCooldown();
    return true;
}

void DemonTHA::UpdateAnimations(float deltaTime) {
    BodyAnimation desiredAnimation = GetDesiredBodyAnimation();
    if (desiredAnimation != bodyAnimation) {
        bodyAnimation = desiredAnimation;
        bodyFrameTimer = 0.0f;
        bodyFrameIndex = 0;
    }

    int bodyFrameCount = IDLE_FRAME_COUNT;
    float bodyFrameDuration = IDLE_FRAME_DURATION;
    if (bodyAnimation == BodyAnimation::Run) {
        bodyFrameCount = RUN_FRAME_COUNT;
        bodyFrameDuration = RUN_FRAME_DURATION;
    } else if (bodyAnimation == BodyAnimation::Shooting) {
        bodyFrameCount = SHOOTING_FRAME_COUNT;
        bodyFrameDuration = SHOOTING_FRAME_DURATION;
    }

    bodyFrameTimer += std::max(0.0f, deltaTime);
    while (bodyFrameTimer >= bodyFrameDuration) {
        bodyFrameTimer -= bodyFrameDuration;
        bodyFrameIndex = (bodyFrameIndex + 1) % bodyFrameCount;
    }

    if (!gunShooting) {
        gunFrameTimer = 0.0f;
        gunFrameIndex = 0;
        return;
    }

    gunFrameTimer += std::max(0.0f, deltaTime);
    while (gunFrameTimer >= GUN_FRAME_DURATION) {
        gunFrameTimer -= GUN_FRAME_DURATION;
        gunFrameIndex =
            (gunFrameIndex + 1) % GUN_SHOOTING_FRAME_COUNT;
        if (gunFrameIndex == GUN_FIRE_FRAME_INDEX) {
            TryFireAtActivePlayer();
        }
    }
}

void DemonTHA::Update(float deltaTime) {
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
    if (health <= 0) return;

    SetAttackCooldown(std::max(
        0.0f,
        GetAttackCooldown() - std::max(0.0f, deltaTime)
    ));
    if (!currentState) ChangeState(wanderIdleState.get());
    if (currentState) currentState->Update(this, deltaTime);

    UpdateMovementAnimationFlag(updateStartPosition);
    UpdateAnimations(deltaTime);
}

void DemonTHA::Draw() {
    if (!ShouldDrawDuringSpawn()) {
        DrawSpawnEffect();
        return;
    }

    Texture2D bodyTexture = idleTexture;
    int bodyFrameCount = IDLE_FRAME_COUNT;
    if (bodyAnimation == BodyAnimation::Run) {
        bodyTexture = runTexture;
        bodyFrameCount = RUN_FRAME_COUNT;
    } else if (bodyAnimation == BodyAnimation::Shooting) {
        bodyTexture = shootingTexture;
        bodyFrameCount = SHOOTING_FRAME_COUNT;
    }

    bool flipSprite = facingLeft != SOURCE_ART_FACES_LEFT;
    float bodyOriginX = flipSprite
        ? BODY_FRAME_WIDTH - 1.0f - BODY_DRAW_ORIGIN.x
        : BODY_DRAW_ORIGIN.x;
    Rectangle bodySource = {
        (float)(bodyFrameIndex % bodyFrameCount) * BODY_FRAME_WIDTH,
        0.0f,
        flipSprite ? -BODY_FRAME_WIDTH : BODY_FRAME_WIDTH,
        BODY_FRAME_HEIGHT
    };
    Color tint = statusComponent.GetStatusTint();
    DrawTexturePro(
        bodyTexture,
        bodySource,
        { position.x, position.y, BODY_FRAME_WIDTH, BODY_FRAME_HEIGHT },
        { bodyOriginX, BODY_DRAW_ORIGIN.y },
        0.0f,
        tint
    );

    if (IsAggroing()) {
        Paladin* target = GetActiveTarget();
        Vector2 aimPosition = target
            ? GetPlayerCenter(*target)
            : Vector2{
                position.x + (facingLeft ? -1.0f : 1.0f),
                position.y
            };
        GunPose pose = CalculateGunPose(position, aimPosition);
        Texture2D gunTexture = gunShooting
            ? shootingGunTexture
            : idleGunTexture;
        int gunFrame = gunShooting ? gunFrameIndex : 0;
        Rectangle gunSource = {
            gunFrame * GUN_FRAME_WIDTH,
            0.0f,
            pose.flipSprite ? -GUN_FRAME_WIDTH : GUN_FRAME_WIDTH,
            GUN_FRAME_HEIGHT
        };
        DrawTexturePro(
            gunTexture,
            gunSource,
            {
                pose.anchorWorld.x,
                pose.anchorWorld.y,
                GUN_FRAME_WIDTH,
                GUN_FRAME_HEIGHT
            },
            pose.drawOrigin,
            pose.angleDegrees,
            tint
        );
    }

    float healthPercent = maxHealth > 0
        ? std::clamp((float)health / (float)maxHealth, 0.0f, 1.0f)
        : 0.0f;
    Rectangle healthBackground = {
        position.x - size.x * 0.5f,
        position.y - size.y * 0.5f - HEALTH_BAR_GAP,
        size.x,
        HEALTH_BAR_HEIGHT
    };
    DrawRectangleRec(healthBackground, Color{ 35, 20, 20, 220 });
    DrawRectangleRec(
        {
            healthBackground.x,
            healthBackground.y,
            healthBackground.width * healthPercent,
            healthBackground.height
        },
        RED
    );
    DrawSpawnEffect();
}
