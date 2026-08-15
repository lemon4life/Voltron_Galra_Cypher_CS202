#include "Entities/EnemyEntities/Boss.h"

#include "Core/Manager/AssetManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/BossFirePunchProjectile.h"
#include "Entities/Player/Paladin.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr int BOSS_MAX_HEALTH = 500;
    constexpr float BOSS_SPEED = 75.0f;
    constexpr int BOSS_DAMAGE = 25;
    constexpr float BOSS_ATTACK_COOLDOWN = 0.8f;
    constexpr int BOSS_IDLE_FRAME_COUNT = 10;
    constexpr int BOSS_RUN_FRAME_COUNT = 6;
    constexpr int BOSS_SPELL_FRAME_COUNT = 6;
    constexpr int BOSS_PUNCH_READY_FRAME_COUNT = 10;
    constexpr int BOSS_PUNCH_BODY_FRAME_COUNT = 4;
    constexpr float BOSS_FRAME_WIDTH = 64.0f;
    constexpr float BOSS_FRAME_HEIGHT = 72.0f;
    constexpr float BOSS_PUNCH_HAND_FRAME_WIDTH = 54.0f;
    constexpr float BOSS_PUNCH_HAND_FRAME_HEIGHT = 14.0f;
    constexpr float BOSS_IDLE_FRAME_DURATION = 0.18f;
    constexpr float BOSS_RUN_FRAME_DURATION = 0.11f;
    constexpr float BOSS_SPELL_FRAME_DURATION = 0.11f;
    constexpr float BOSS_KNOCKBACK_RESISTANCE = 1.0f;

    // Across the ten idle and six running frames, visible pixels occupy the
    // combined x=8..54 and y=3..71 bounds inside each 64x72 cell.
    constexpr Vector2 BOSS_SIZE = { 47.0f, 69.0f };
    constexpr Vector2 BOSS_DRAW_ORIGIN = { 31.5f, 37.5f };
    constexpr Vector2 BOSS_PUNCH_BODY_HAND_ROOT = { 13.0f, 43.0f };
    constexpr Vector2 BOSS_PUNCH_HAND_ROOT = { 8.0f, 7.0f };
    constexpr Vector2 BOSS_PUNCH_HAND_LAUNCH_PIXEL = { 53.0f, 7.0f };
    constexpr Vector2 BOSS_RENDER_FOOT_OFFSET = {
        0.0f,
        BOSS_FRAME_HEIGHT - BOSS_DRAW_ORIGIN.y
    };

    // The bottom twelve source rows contain the stable foot silhouette. Its
    // center is 28.5 pixels below the recalculated visible-body center.
    constexpr EnemyCollisionProfile BOSS_COLLISION_PROFILE = {
        { 32.0f, 12.0f },
        { 0.0f, 28.5f }
    };

    constexpr float BOSS_HEALTH_BAR_WIDTH = BOSS_FRAME_WIDTH;
    constexpr float BOSS_HEALTH_BAR_HEIGHT = 6.0f;
    constexpr float BOSS_HEALTH_BAR_GAP = 8.0f;
    constexpr int BOSS_SUMMON_ATTEMPTS = 12;
    constexpr int BOSS_SUMMON_MIN_RADIUS_TENTHS = 560;
    constexpr int BOSS_SUMMON_MAX_RADIUS_TENTHS = 960;

    struct BossPunchHandPose {
        Vector2 anchorWorld;
        Vector2 origin;
        float angleDegrees;
    };

    BossPunchHandPose CalculatePunchHandPose(
        Vector2 bossPosition,
        bool flipSprite,
        const Paladin* target
    ) {
        Vector2 bodyDrawPosition = {
            std::round(bossPosition.x),
            std::round(bossPosition.y)
        };
        float bodyRootX = flipSprite
            ? BOSS_FRAME_WIDTH - 1.0f - BOSS_PUNCH_BODY_HAND_ROOT.x
            : BOSS_PUNCH_BODY_HAND_ROOT.x;
        Vector2 handAnchorWorld = {
            bodyDrawPosition.x - BOSS_DRAW_ORIGIN.x + bodyRootX,
            bodyDrawPosition.y - BOSS_DRAW_ORIGIN.y +
                BOSS_PUNCH_BODY_HAND_ROOT.y
        };

        float handAngle = flipSprite ? 180.0f : 0.0f;
        if (target) {
            Vector2 targetDirection = Vector2Subtract(
                target->GetPosition(),
                handAnchorWorld
            );
            if (Vector2Length(targetDirection) > 0.0f) {
                handAngle = std::atan2(
                    targetDirection.y,
                    targetDirection.x
                ) * RAD2DEG + (flipSprite ? 180.0f : 0.0f);
            }
        }

        return {
            handAnchorWorld,
            {
                flipSprite
                    ? BOSS_PUNCH_HAND_FRAME_WIDTH - 1.0f -
                        BOSS_PUNCH_HAND_ROOT.x
                    : BOSS_PUNCH_HAND_ROOT.x,
                BOSS_PUNCH_HAND_ROOT.y
            },
            handAngle
        };
    }

    Vector2 RotatePunchOffset(Vector2 offset, float angleDegrees) {
        float angleRadians = angleDegrees * DEG2RAD;
        float cosine = std::cos(angleRadians);
        float sine = std::sin(angleRadians);
        return {
            offset.x * cosine - offset.y * sine,
            offset.x * sine + offset.y * cosine
        };
    }

    Vector2 CalculateFirePunchLaunchPosition(
        const BossPunchHandPose& handPose,
        bool flipSprite
    ) {
        Vector2 mirroredLaunchPixel = {
            flipSprite
                ? BOSS_PUNCH_HAND_FRAME_WIDTH - 1.0f -
                    BOSS_PUNCH_HAND_LAUNCH_PIXEL.x
                : BOSS_PUNCH_HAND_LAUNCH_PIXEL.x,
            BOSS_PUNCH_HAND_LAUNCH_PIXEL.y
        };
        Vector2 launchOffset = {
            mirroredLaunchPixel.x - handPose.origin.x,
            mirroredLaunchPixel.y - handPose.origin.y
        };
        Vector2 rotatedOffset = RotatePunchOffset(
            launchOffset,
            handPose.angleDegrees
        );
        return {
            handPose.anchorWorld.x + rotatedOffset.x,
            handPose.anchorWorld.y + rotatedOffset.y
        };
    }
}

Boss::Boss(
    Vector2 pos,
    TeamManager* targetTeam,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess
)
    : Enemy(
          pos,
          targetTeam,
          BOSS_MAX_HEALTH,
          BOSS_SPEED,
          BOSS_DAMAGE,
          BOSS_ATTACK_COOLDOWN,
          removalAccess,
          pathAccess
      )
{
    enemyType = EnemyType::BOSS;
    size = BOSS_SIZE;
    SetRenderFootOffset(BOSS_RENDER_FOOT_OFFSET);
    SetCollisionProfile(BOSS_COLLISION_PROFILE);
    SetKnockbackResistance(BOSS_KNOCKBACK_RESISTANCE);

    idleState = std::make_unique<BossIdlingState>();
    chaseState = std::make_unique<BossChaseState>();
    spellingState = std::make_unique<BossSpellingState>();
    punchState = std::make_unique<BossPunchState>();

    SetEnemySprites(AssetManager::GetInstance().GetBossSprites());
    spellTexture = AssetManager::GetInstance().GetTexture("Boss_Spell");
    punchReadyTexture = AssetManager::GetInstance().GetTexture(
        "Boss_Punch_Ready"
    );
    punchBodyTexture = AssetManager::GetInstance().GetTexture(
        "Boss_Punch_Body"
    );
    punchHandTexture = AssetManager::GetInstance().GetTexture(
        "Boss_Punch_Hand"
    );
    firePunchTexture = AssetManager::GetInstance().GetTexture(
        "Boss_Fire_Punch"
    );

    ChangeState(GetIdlingState());
}

Boss::~Boss() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void Boss::Update(float deltaTime) {
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

    if (health <= 0) return;

    UpdateMovementAnimationFlag(updateStartPosition);

    if (targetTeam && targetTeam->GetActivePaladin()) {
        facingLeft = targetTeam->GetActivePaladin()->GetPosition().x <
            position.x;
    }

    if (IsPunching()) {
        return;
    }

    bool useSpellAnimation = IsSpelling();
    bool useRunAnimation = !useSpellAnimation && IsMovingForAnimation();
    int frameCount = useSpellAnimation
        ? BOSS_SPELL_FRAME_COUNT
        : (useRunAnimation ? BOSS_RUN_FRAME_COUNT : BOSS_IDLE_FRAME_COUNT);
    float frameDuration = useSpellAnimation
        ? BOSS_SPELL_FRAME_DURATION
        : (useRunAnimation ? BOSS_RUN_FRAME_DURATION : BOSS_IDLE_FRAME_DURATION);
    currentRunFrame %= frameCount;
    runFrameTime += deltaTime;
    while (runFrameTime >= frameDuration) {
        runFrameTime -= frameDuration;
        currentRunFrame = (currentRunFrame + 1) % frameCount;
    }
}

void Boss::Draw() {
    if (!ShouldDrawDuringSpawn()) {
        DrawSpawnEffect();
        return;
    }

    bool flipSprite = facingLeft;
    Vector2 bodyDrawPosition = {
        std::round(position.x),
        std::round(position.y)
    };
    Color tint = statusComponent.GetStatusTint();

    auto DrawBossBodyFrame = [&](Texture2D texture,
                                 int frameIndex,
                                 int frameCount) {
        if (texture.id == 0 ||
            texture.width < (int)(BOSS_FRAME_WIDTH * frameCount) ||
            texture.height < (int)BOSS_FRAME_HEIGHT) {
            return false;
        }

        Rectangle source = {
            frameIndex * BOSS_FRAME_WIDTH,
            0.0f,
            flipSprite ? -BOSS_FRAME_WIDTH : BOSS_FRAME_WIDTH,
            BOSS_FRAME_HEIGHT
        };
        Rectangle destination = {
            bodyDrawPosition.x,
            bodyDrawPosition.y,
            BOSS_FRAME_WIDTH,
            BOSS_FRAME_HEIGHT
        };
        DrawTexturePro(
            texture,
            source,
            destination,
            BOSS_DRAW_ORIGIN,
            0.0f,
            tint
        );
        return true;
    };

    bool drewBody = false;
    if (IsPunching()) {
        BossPunchState* punchAnimation = GetPunchState();
        int punchFrame = punchAnimation->GetFrameIndex();
        bool isReady = punchAnimation->GetPhase() ==
            BossPunchState::Phase::Ready;

        drewBody = DrawBossBodyFrame(
            isReady ? punchReadyTexture : punchBodyTexture,
            punchFrame,
            isReady
                ? BOSS_PUNCH_READY_FRAME_COUNT
                : BOSS_PUNCH_BODY_FRAME_COUNT
        );

        bool validHandTexture = punchHandTexture.id != 0 &&
            punchHandTexture.width >= (int)(
                BOSS_PUNCH_HAND_FRAME_WIDTH * BOSS_PUNCH_BODY_FRAME_COUNT
            ) &&
            punchHandTexture.height >=
                (int)BOSS_PUNCH_HAND_FRAME_HEIGHT;
        if (!isReady && validHandTexture) {
            Paladin* activePaladin = targetTeam
                ? targetTeam->GetActivePaladin()
                : nullptr;
            BossPunchHandPose handPose = CalculatePunchHandPose(
                position,
                flipSprite,
                activePaladin
            );

            DrawTexturePro(
                punchHandTexture,
                {
                    punchFrame * BOSS_PUNCH_HAND_FRAME_WIDTH,
                    0.0f,
                    flipSprite
                        ? -BOSS_PUNCH_HAND_FRAME_WIDTH
                        : BOSS_PUNCH_HAND_FRAME_WIDTH,
                    BOSS_PUNCH_HAND_FRAME_HEIGHT
                },
                {
                    handPose.anchorWorld.x,
                    handPose.anchorWorld.y,
                    BOSS_PUNCH_HAND_FRAME_WIDTH,
                    BOSS_PUNCH_HAND_FRAME_HEIGHT
                },
                handPose.origin,
                handPose.angleDegrees,
                tint
            );
        }
    } else {
        bool useSpellAnimation = IsSpelling();
        bool useRunAnimation = !useSpellAnimation && IsMovingForAnimation();
        int frameCount = useSpellAnimation
            ? BOSS_SPELL_FRAME_COUNT
            : (useRunAnimation
                ? BOSS_RUN_FRAME_COUNT
                : BOSS_IDLE_FRAME_COUNT);
        int frameIndex = currentRunFrame % frameCount;
        Texture2D texture = useSpellAnimation
            ? spellTexture
            : (useRunAnimation ? sprites.run : sprites.idle);

        drewBody = DrawBossBodyFrame(texture, frameIndex, frameCount);
    }

    if (!drewBody) {
        DrawRectangleRec(GetBoundingBox(), ORANGE);
    }

    float healthPercent = maxHealth > 0
        ? std::clamp((float)health / (float)maxHealth, 0.0f, 1.0f)
        : 0.0f;
    Rectangle healthBar = {
        position.x - BOSS_HEALTH_BAR_WIDTH * 0.5f,
        position.y - BOSS_SIZE.y * 0.5f - BOSS_HEALTH_BAR_GAP,
        BOSS_HEALTH_BAR_WIDTH,
        BOSS_HEALTH_BAR_HEIGHT
    };
    DrawRectangleRec(healthBar, Color{ 24, 24, 28, 220 });
    DrawRectangleRec(
        {
            healthBar.x,
            healthBar.y,
            healthBar.width * healthPercent,
            healthBar.height
        },
        RED
    );
    DrawSpawnEffect();
}

bool Boss::TrySummonRandomEnemy() {
    LevelManager* levelManager =
        GameManager::GetInstance().GetLevelManager();
    if (!levelManager || !targetTeam) return false;

    constexpr MapObjectId SUMMON_TYPES[] = {
        MapObjectId::Chaser,
        MapObjectId::Range,
        MapObjectId::Diver
    };
    MapObjectId summonType = SUMMON_TYPES[GetRandomValue(0, 2)];

    for (int attempt = 0; attempt < BOSS_SUMMON_ATTEMPTS; ++attempt) {
        float angle = (float)GetRandomValue(0, 359) * DEG2RAD;
        float radius = (float)GetRandomValue(
            BOSS_SUMMON_MIN_RADIUS_TENTHS,
            BOSS_SUMMON_MAX_RADIUS_TENTHS
        ) / 10.0f;
        Vector2 summonPosition = {
            position.x + std::cos(angle) * radius,
            position.y + std::sin(angle) * radius
        };
        if (levelManager->QueueEnemySpawn(
                summonType,
                summonPosition,
                targetTeam)) {
            return true;
        }
    }

    return false;
}

void Boss::FirePunchProjectile(
    float bulletSpeed,
    float changeAngleDegreesPerSecond
) {
    if (!targetTeam || firePunchTexture.id == 0) return;

    Paladin* activePaladin = targetTeam->GetActivePaladin();
    if (!activePaladin) return;

    BossPunchHandPose handPose = CalculatePunchHandPose(
        position,
        facingLeft,
        activePaladin
    );
    Vector2 launchPosition = CalculateFirePunchLaunchPosition(
        handPose,
        facingLeft
    );

    GameManager& gameManager = GameManager::GetInstance();
    LevelManager* levelManager = gameManager.GetLevelManager();
    Rectangle mapBounds = levelManager
        ? levelManager->GetLevelBounds()
        : Rectangle{
            0.0f,
            0.0f,
            gameManager.GetLevelWidth(),
            gameManager.GetLevelHeight()
        };
    Rectangle roomBounds = levelManager
        ? levelManager->GetCurrentRoomBounds()
        : mapBounds;

    gameManager.AddProjectile(new BossFirePunchProjectile(
        launchPosition,
        activePaladin->GetPosition(),
        targetTeam,
        bulletSpeed,
        changeAngleDegreesPerSecond,
        GetDamage(),
        firePunchTexture,
        roomBounds,
        mapBounds
    ));
}

void Boss::ResetAnimationCycle() {
    currentRunFrame = 0;
    runFrameTime = 0.0f;
}
