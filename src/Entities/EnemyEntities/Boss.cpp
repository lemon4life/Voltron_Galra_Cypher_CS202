#include "Entities/EnemyEntities/Boss.h"

#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/BossIntroManager.h"
#include "Core/Manager/CameraManager.h"
#include "Core/Constants.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/BossFirePunchProjectile.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Projectile.h"
#include "Entities/Projectiles/DroneBullet.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr int BOSS_MAX_HEALTH = 3000;
    constexpr int BOSS_PHASE_TWO_MAX_HEALTH = 2000;
    constexpr int BOSS_PHASE_THREE_MAX_HEALTH = 1000;
    constexpr float BOSS_PHASE_TWO_IDLE_DAMAGE_SCALE = 0.80f;
    constexpr float BOSS_PHASE_THREE_IDLE_DAMAGE_SCALE = 0.60f;
    constexpr int BOSS_PHASE_TWO_CLONE_HEALTH = 700;
    constexpr int BOSS_PHASE_THREE_CLONE_HEALTH = 1200;
    constexpr int BOSS_BASE_IDLE_MIN_MILLISECONDS = 2000;
    constexpr int BOSS_BASE_IDLE_MAX_MILLISECONDS = 3000;
    constexpr float BOSS_PHASE_TWO_IDLE_DURATION_SCALE = 0.70f;
    constexpr float BOSS_PHASE_THREE_IDLE_DURATION_SCALE = 0.40f;
    constexpr float BOSS_PHASE_TWO_MOVEMENT_SPEED_SCALE = 1.30f;
    constexpr float BOSS_PHASE_THREE_MOVEMENT_SPEED_SCALE = 1.50f;
    constexpr int BOSS_PHASE_ONE_STOMPS = 2;
    constexpr int BOSS_HARDER_PHASE_STOMPS = 3;
    constexpr int BOSS_PHASE_ONE_PUNCHES = 4;
    constexpr int BOSS_PHASE_TWO_PUNCHES = 7;
    constexpr int BOSS_PHASE_THREE_PUNCHES = 10;
    constexpr float BOSS_NORMAL_SPELL_SUMMON_INTERVAL = 0.5f;
    constexpr float BOSS_PHASE_THREE_SPELL_SUMMON_INTERVAL = 0.2f;
    constexpr int BOSS_PHASE_ONE_SUMMON_CHANCE_PERCENT = 50;
    constexpr int BOSS_HARDER_PHASE_SUMMON_CHANCE_PERCENT = 70;
    constexpr int BOSS_PHASE_THREE_MAX_DEMON_SUMMONS = 2;
    constexpr std::size_t BOSS_RANDOM_SPELL_MAX_LIVING_ENEMIES = 70;
    constexpr float BOSS_PHASE_THREE_STOMP_BULLET_SPEED_SCALE = 1.5f;
    constexpr float BOSS_SPEED = 75.0f;
    constexpr int BOSS_DAMAGE = 25;
    constexpr float BOSS_ATTACK_COOLDOWN = 0.8f;
    constexpr int BOSS_IDLE_FRAME_COUNT = 10;
    constexpr int BOSS_RUN_FRAME_COUNT = 6;
    constexpr int BOSS_SPELL_FRAME_COUNT = 6;
    constexpr int BOSS_PUNCH_READY_FRAME_COUNT = 10;
    constexpr int BOSS_PUNCH_BODY_FRAME_COUNT = 4;
    constexpr int BOSS_STOMP_FRAME_COUNT = 5;
    constexpr float BOSS_FRAME_WIDTH = 64.0f;
    constexpr float BOSS_FRAME_HEIGHT = 72.0f;
    constexpr float BOSS_PUNCH_HAND_FRAME_WIDTH = 54.0f;
    constexpr float BOSS_PUNCH_HAND_FRAME_HEIGHT = 14.0f;
    constexpr float BOSS_IDLE_FRAME_DURATION = 0.18f;
    constexpr float BOSS_RUN_FRAME_DURATION = 0.11f;
    constexpr float BOSS_SPELL_FRAME_DURATION = 0.11f;
    constexpr float BOSS_KNOCKBACK_RESISTANCE = 1.0f;
    constexpr int BOSS_STOMP_SMOKE_FRAME_COUNT = 9;
    constexpr float BOSS_STOMP_SMOKE_FRAME_DURATION = 0.10f;
    constexpr Vector2 BOSS_STOMP_FOOT_PIXEL = { 41.0f, 71.0f };
    constexpr Vector2 BOSS_STOMP_SMOKE_ORIGIN = { 47.0f, 44.0f };
    constexpr int BOSS_STOMP_DRONE_BULLET_COUNT = 36;
    constexpr int BOSS_STOMP_KNIGHT_BULLET_COUNT = 12;
    constexpr float BOSS_STOMP_PROJECTILE_SPAWN_RADIUS = 15.0f;
    constexpr float STOMP_DRONE_BULLET_INITIAL_SPEED = 200.0f;
    constexpr float STOMP_DRONE_BULLET_MINIMUM_SPEED = 40.0f;
    constexpr float STOMP_DRONE_BULLET_DRAG = 400.0f;
    constexpr float STOMP_DRONE_BULLET_LIFETIME = 4.0f;
    constexpr float STOMP_DRONE_BULLET_RADIUS = 4.0f;
    constexpr int STOMP_DRONE_BULLET_DAMAGE = 15;
    constexpr float STOMP_KNIGHT_BULLET_SPEED = 160.0f;
    constexpr float STOMP_KNIGHT_BULLET_LIFETIME = 2.0f;
    constexpr float STOMP_KNIGHT_BULLET_RADIUS = 5.0f;
    constexpr int STOMP_KNIGHT_BULLET_DAMAGE = 12;

    // Across the ten idle and six running frames, visible pixels occupy the
    // combined x=8..54 and y=3..71 bounds inside each 64x72 cell.
    constexpr Vector2 BOSS_SIZE = { 47.0f, 69.0f };
    constexpr Vector2 BOSS_DRAW_ORIGIN = { 31.5f, 37.5f };
    constexpr Vector2 BOSS_PUNCH_BODY_HAND_ROOT = { 13.0f, 43.0f };
    constexpr Vector2 BOSS_PUNCH_HAND_ROOT = { 8.0f, 7.0f };
    constexpr Vector2 BOSS_PUNCH_HAND_LAUNCH_PIXEL = { 38.0f, 7.0f };
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
    constexpr float BOSS_SUMMON_CORRECTION_RADIUS =
        Constants::RENDER_TILE_SIZE * 3.0f;
    constexpr float BOSS_CLONE_SUMMON_CORRECTION_RADIUS =
        Constants::RENDER_TILE_SIZE * 6.0f;
    constexpr Color BOSS_PHASE_TWO_TINT = { 255, 215, 215, 255 };
    constexpr Color BOSS_PHASE_THREE_TINT = { 255, 165, 165, 255 };
    constexpr Color BOSS_CLONE_TINT = { 145, 190, 255, 255 };
    constexpr int BOSS_CINEMATIC_STOMP_COUNT = 3;
    constexpr float BOSS_STOMP_SHAKE_DURATION = 0.2f;
    constexpr float BOSS_STOMP_SHAKE_BASE_MAGNITUDE = 4.0f;
    constexpr float BOSS_STOMP_SHAKE_PHASE_INCREMENT = 0.10f;
    constexpr float BOSS_INTRO_CAMERA_WIDTH = 120.0f;
    constexpr float BOSS_INTRO_CAMERA_HEIGHT = 120.0f;
    constexpr float BOSS_PHASE_CAMERA_HALF_EXTENT = 112.0f;

    /// Combines two color tints by multiplying their normalized channels.
    Color MultiplyColor(Color first, Color second) {
        return {
            static_cast<unsigned char>(
                static_cast<unsigned int>(first.r) * second.r / 255U
            ),
            static_cast<unsigned char>(
                static_cast<unsigned int>(first.g) * second.g / 255U
            ),
            static_cast<unsigned char>(
                static_cast<unsigned int>(first.b) * second.b / 255U
            ),
            static_cast<unsigned char>(
                static_cast<unsigned int>(first.a) * second.a / 255U
            )
        };
    }

    struct BossPunchHandPose {
        Vector2 anchorWorld;
        Vector2 origin;
        float angleDegrees;
    };

    /// Calculates punch hand pose.
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

    /// Rotates the boss punch attachment offset to match its facing direction.
    Vector2 RotatePunchOffset(Vector2 offset, float angleDegrees) {
        float angleRadians = angleDegrees * DEG2RAD;
        float cosine = std::cos(angleRadians);
        float sine = std::sin(angleRadians);
        return {
            offset.x * cosine - offset.y * sine,
            offset.x * sine + offset.y * cosine
        };
    }

    /// Calculates fire punch launch position.
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

/// Creates a Boss instance from the supplied configuration.
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
    spellingState = std::make_unique<BossSpellingState>();
    punchState = std::make_unique<BossPunchState>();
    stompingState = std::make_unique<BossStompingState>();

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
    stompTexture = AssetManager::GetInstance().GetTexture("Boss_Stomp");
    stompSmokeTexture = AssetManager::GetInstance().GetTexture(
        "Boss_Stomp_Smoke"
    );
    stompDroneBulletTexture = AssetManager::GetInstance().GetTexture(
        "Drone_bullet"
    );
    stompKnightBulletTexture = AssetManager::GetInstance().GetTexture(
        "Knight_Gun_Bullet"
    );

    ChangeState(GetIdlingState());
}

/// Releases resources owned by this Boss instance.
Boss::~Boss() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

/// Advances spawn/death handling, phase transitions, active AI state, status
/// effects, and movement animation. State objects own offense-specific timing;
/// Boss keeps the shared phase and visual bookkeeping consistent around them.
void Boss::Update(float deltaTime) {
    Vector2 updateStartPosition = position;
    if (UpdateSpawnSequence(deltaTime)) {
        UpdateMovementAnimationFlag(updateStartPosition);
        return;
    }
    if (cinematicStage == BossCinematicStage::Introduction) {
        if (!introPlayed) {
            introPlayed = true;
            BossIntroManager::GetInstance().PlayIntro(this);
            SetCurrentVelocity({ 0.0f, 0.0f });
            UpdateMovementAnimationFlag(updateStartPosition);
            return;
        }

        if (BossIntroManager::GetInstance().IsPlaying()) {
            SetCurrentVelocity({ 0.0f, 0.0f });
            UpdateMovementAnimationFlag(updateStartPosition);
            return;
        }

        // The spawn sequence and intro banner are now fully complete.
        cinematicStage = BossCinematicStage::None;
        SetCurrentVelocity({ 0.0f, 0.0f });
        UpdateMovementAnimationFlag(updateStartPosition);
        return;
    }
    UpdateKnockback(deltaTime);
    
    if (statusComponent.Update(deltaTime, this)) {
        UpdateMovementAnimationFlag(updateStartPosition);
        return;
    }

    EvaluatePhaseTransitions();
    
    if (currentState) {
        currentState->Update(this, deltaTime);
    }

    if (health <= 0) return;

    UpdateMovementAnimationFlag(updateStartPosition);

    if (targetTeam && targetTeam->GetActivePaladin()) {
        facingLeft = targetTeam->GetActivePaladin()->GetPosition().x <
            position.x;
    }

    if (IsPunching() || IsStomping()) {
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

/// Renders this component using its current state and visual resources.
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
    Color tint = MultiplyColor(
        statusComponent.GetStatusTint(),
        GetBodyTint()
    );

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
    if (IsStomping()) {
        drewBody = DrawBossBodyFrame(
            stompTexture,
            GetStompingState()->GetFrameIndex(),
            BOSS_STOMP_FRAME_COUNT
        );
    } else if (IsPunching()) {
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

    // Overhead health bar removed - now rendered in dedicated fixed top HUD (Soul Knight style)
    DrawSpawnEffect();
}

/// Returns the current phase.
BossPhase Boss::GetPhase() const {
    if (phaseLocked) return lockedPhase;
    if (health > BOSS_PHASE_TWO_MAX_HEALTH) return BossPhase::Phase1;
    if (health > BOSS_PHASE_THREE_MAX_HEALTH) return BossPhase::Phase2;
    return BossPhase::Phase3;
}

/// Restricts only normal offense selection; forced cinematic spells bypass it.
bool Boss::CanSelectRandomSpell() const {
    std::size_t livingEnemyCount = 0;
    for (const Enemy* enemy : GameManager::GetInstance()
             .GetObjectManager()
             .GetEnemies()) {
        if (enemy && !enemy->IsDead()) ++livingEnemyCount;
    }
    return livingEnemyCount <= BOSS_RANDOM_SPELL_MAX_LIVING_ENEMIES;
}

/// Applies incoming damage after this object handles defenses and state-specific rules.
void Boss::TakeDamage(int amount) {
    if (amount <= 0 || health <= 0) return;

    float damageScale = 1.0f;
    if (!IsInOffensiveState()) {
        if (GetPhase() == BossPhase::Phase2) {
            damageScale = BOSS_PHASE_TWO_IDLE_DAMAGE_SCALE;
        } else if (GetPhase() == BossPhase::Phase3) {
            damageScale = BOSS_PHASE_THREE_IDLE_DAMAGE_SCALE;
        }
    }

    int scaledDamage = std::max(
        1,
        static_cast<int>(std::lround(amount * damageScale))
    );
    Enemy::TakeDamage(scaledDamage);
    if (health > 0) EvaluatePhaseTransitions();
}

/// Configures as clone.
void Boss::ConfigureAsClone(int cloneHealth, BossPhase clonePhase) {
    phaseLocked = true;
    lockedPhase = clonePhase;
    cloneBoss = true;
    phaseTwoTransitionTriggered = true;
    phaseThreeTransitionTriggered = true;
    phaseOneClonePending = false;
    phaseTwoClonePending = false;
    cinematicStage = BossCinematicStage::None;
    SetMaxHealth(std::max(1, cloneHealth));
    SetHealth(GetMaxHealth());
}

/// Detects boss health thresholds and starts each one-time phase transition.
void Boss::EvaluatePhaseTransitions() {
    if (phaseLocked || health <= 0) return;

    BossPhase phase = GetPhase();
    bool enteredNewPhase = false;
    if (phase != BossPhase::Phase1 && !phaseTwoTransitionTriggered) {
        phaseTwoTransitionTriggered = true;
        phaseOneClonePending = true;
        enteredNewPhase = true;
    }
    if (phase == BossPhase::Phase3 && !phaseThreeTransitionTriggered) {
        phaseThreeTransitionTriggered = true;
        phaseTwoClonePending = true;
        enteredNewPhase = true;
    }

    if (enteredNewPhase) StartPhaseCinematic();
}

/// Stops normal boss behavior and begins the scripted three-stomp phase ceremony.
void Boss::StartPhaseCinematic() {
    cinematicStage = BossCinematicStage::PhaseStomps;
    EndPathFinding();
    SetCurrentVelocity({ 0.0f, 0.0f });
    statusComponent.Clear();
    ChangeState(GetStompingState());
}

/// Returns the current body tint.
Color Boss::GetBodyTint() const {
    if (cloneBoss) return BOSS_CLONE_TINT;

    switch (GetPhase()) {
        case BossPhase::Phase2:
            return BOSS_PHASE_TWO_TINT;
        case BossPhase::Phase3:
            return BOSS_PHASE_THREE_TINT;
        case BossPhase::Phase1:
        default:
            return WHITE;
    }
}

/// Attempts to summon boss clone.
bool Boss::TrySummonBossClone(int cloneHealth, BossPhase clonePhase) {
    LevelManager* levelManager =
        GameManager::GetInstance().GetLevelManager();
    if (!levelManager || !targetTeam) return false;

    std::shared_ptr<RoomNode> lockedRoom =
        levelManager->GetCurrentlyLockedRoom();
    if (!lockedRoom || lockedRoom->type != RoomType::BOSS ||
        lockedRoom->state != RoomState::LOCKED) {
        return false;
    }

    float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    float radius = static_cast<float>(GetRandomValue(
        BOSS_SUMMON_MIN_RADIUS_TENTHS,
        BOSS_SUMMON_MAX_RADIUS_TENTHS
    )) / 10.0f;
    Vector2 desiredPosition = {
        position.x + std::cos(angle) * radius,
        position.y + std::sin(angle) * radius
    };

    return GameManager::GetInstance()
        .GetObjectManager()
        .QueueEnemySpawnSafely(
            MapObjectId::Boss,
            desiredPosition,
            lockedRoom->GetWorldBounds(),
            BOSS_CLONE_SUMMON_CORRECTION_RADIUS,
            [cloneHealth, clonePhase](Enemy& enemy) {
                Boss* clone = dynamic_cast<Boss*>(&enemy);
                if (clone) {
                    clone->ConfigureAsClone(cloneHealth, clonePhase);
                }
            }
        );
}

/// Attempts to summon pending phase clones.
void Boss::TrySummonPendingPhaseClones() {
    if (phaseOneClonePending && TrySummonBossClone(
            BOSS_PHASE_TWO_CLONE_HEALTH,
            BossPhase::Phase1
        )) {
        phaseOneClonePending = false;
    }

    if (phaseTwoClonePending && TrySummonBossClone(
            BOSS_PHASE_THREE_CLONE_HEALTH,
            BossPhase::Phase2
        )) {
        phaseTwoClonePending = false;
    }
}

/// Returns the current idle minimum milliseconds.
int Boss::GetIdleMinimumMilliseconds() const {
    float scale = 1.0f;
    if (GetPhase() == BossPhase::Phase2) {
        scale = BOSS_PHASE_TWO_IDLE_DURATION_SCALE;
    } else if (GetPhase() == BossPhase::Phase3) {
        scale = BOSS_PHASE_THREE_IDLE_DURATION_SCALE;
    }
    return (int)(BOSS_BASE_IDLE_MIN_MILLISECONDS * scale);
}

/// Returns the current idle maximum milliseconds.
int Boss::GetIdleMaximumMilliseconds() const {
    float scale = 1.0f;
    if (GetPhase() == BossPhase::Phase2) {
        scale = BOSS_PHASE_TWO_IDLE_DURATION_SCALE;
    } else if (GetPhase() == BossPhase::Phase3) {
        scale = BOSS_PHASE_THREE_IDLE_DURATION_SCALE;
    }
    return (int)(BOSS_BASE_IDLE_MAX_MILLISECONDS * scale);
}

/// Returns the current idle movement speed scale.
float Boss::GetIdleMovementSpeedScale() const {
    switch (GetPhase()) {
        case BossPhase::Phase2:
            return BOSS_PHASE_TWO_MOVEMENT_SPEED_SCALE;
        case BossPhase::Phase3:
            return BOSS_PHASE_THREE_MOVEMENT_SPEED_SCALE;
        case BossPhase::Phase1:
        default:
            return 1.0f;
    }
}

/// Returns the current stomps per state.
int Boss::GetStompsPerState() const {
    return GetPhase() == BossPhase::Phase1
        ? BOSS_PHASE_ONE_STOMPS
        : BOSS_HARDER_PHASE_STOMPS;
}

/// Returns three non-damaging stomps during a phase ceremony, otherwise the
/// phase's normal offensive stomp count.
int Boss::GetCurrentStompCount() const {
    return cinematicStage == BossCinematicStage::PhaseStomps
        ? BOSS_CINEMATIC_STOMP_COUNT
        : GetStompsPerState();
}

/// Returns the current punches per state.
int Boss::GetPunchesPerState() const {
    switch (GetPhase()) {
        case BossPhase::Phase1:
            return BOSS_PHASE_ONE_PUNCHES;
        case BossPhase::Phase2:
            return BOSS_PHASE_TWO_PUNCHES;
        case BossPhase::Phase3:
        default:
            return BOSS_PHASE_THREE_PUNCHES;
    }
}

/// Returns the current spell summon interval.
float Boss::GetSpellSummonInterval() const {
    return GetPhase() == BossPhase::Phase3
        ? BOSS_PHASE_THREE_SPELL_SUMMON_INTERVAL
        : BOSS_NORMAL_SPELL_SUMMON_INTERVAL;
}

/// Returns the current spell summon chance percent.
int Boss::GetSpellSummonChancePercent() const {
    return GetPhase() == BossPhase::Phase1
        ? BOSS_PHASE_ONE_SUMMON_CHANCE_PERCENT
        : BOSS_HARDER_PHASE_SUMMON_CHANCE_PERCENT;
}

/// Attempts to summon random enemy.
bool Boss::TrySummonRandomEnemy(int& demonsSummonedThisSpell) {
    LevelManager* levelManager =
        GameManager::GetInstance().GetLevelManager();
    if (!levelManager || !targetTeam) return false;
    std::shared_ptr<RoomNode> lockedRoom =
        levelManager->GetCurrentlyLockedRoom();
    if (!lockedRoom || lockedRoom->type != RoomType::BOSS ||
        lockedRoom->state != RoomState::LOCKED) {
        return false;
    }

    constexpr MapObjectId SUMMON_TYPES[] = {
        MapObjectId::Chaser,
        MapObjectId::Range,
        MapObjectId::Diver,
        MapObjectId::DemonTHA
    };
    BossPhase phase = GetPhase();
    bool canSummonDemon = phase != BossPhase::Phase1 &&
        (phase != BossPhase::Phase3 ||
         demonsSummonedThisSpell < BOSS_PHASE_THREE_MAX_DEMON_SUMMONS);
    int maximumTypeIndex = canSummonDemon ? 3 : 2;
    MapObjectId summonType = SUMMON_TYPES[
        GetRandomValue(0, maximumTypeIndex)
    ];

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
        if (GameManager::GetInstance()
                .GetObjectManager()
                .QueueEnemySpawnSafely(
                    summonType,
                    summonPosition,
                    lockedRoom->GetWorldBounds(),
                    BOSS_SUMMON_CORRECTION_RADIUS
                )) {
            if (summonType == MapObjectId::DemonTHA) {
                ++demonsSummonedThisSpell;
            }
            return true;
        }
    }

    return false;
}

/// Returns the current stomp foot world position.
Vector2 Boss::GetStompFootWorldPosition() const {
    Vector2 bodyDrawPosition = {
        std::round(position.x),
        std::round(position.y)
    };
    float footPixelX = facingLeft
        ? BOSS_FRAME_WIDTH - 1.0f - BOSS_STOMP_FOOT_PIXEL.x
        : BOSS_STOMP_FOOT_PIXEL.x;
    return {
        bodyDrawPosition.x + footPixelX - BOSS_DRAW_ORIGIN.x,
        bodyDrawPosition.y + BOSS_STOMP_FOOT_PIXEL.y - BOSS_DRAW_ORIGIN.y
    };
}

/// Spawns stomp smoke.
void Boss::SpawnStompSmoke() {
    if (stompSmokeTexture.id == 0) return;

    GameManager::GetInstance().GetEffectManager().AddAnchoredEffect(
        GetStompFootWorldPosition(),
        stompSmokeTexture,
        BOSS_STOMP_SMOKE_FRAME_COUNT,
        BOSS_STOMP_SMOKE_FRAME_COUNT *
            BOSS_STOMP_SMOKE_FRAME_DURATION,
        BOSS_STOMP_SMOKE_ORIGIN
    );
}

/// Plays one stomp impact. Cinematic stomps intentionally retain their smoke,
/// sound, and shake while suppressing the radial projectile attack.
void Boss::HandleStompImpact() {
    AudioManager::GetInstance().PlaySoundEffect("boss_stomping");
    SpawnStompSmoke();

    int phaseIndex = static_cast<int>(GetPhase());
    float shakeMagnitude = BOSS_STOMP_SHAKE_BASE_MAGNITUDE *
        (1.0f + BOSS_STOMP_SHAKE_PHASE_INCREMENT * phaseIndex);
    CameraManager::GetInstance().StartShake(
        BOSS_STOMP_SHAKE_DURATION,
        shakeMagnitude
    );

    if (cinematicStage != BossCinematicStage::PhaseStomps) {
        FireStompProjectiles();
    }
}

/// Continues a phase ceremony into its forced spell; normal stomps return to idle.
void Boss::CompleteStompingState() {
    if (cinematicStage == BossCinematicStage::PhaseStomps) {
        cinematicStage = BossCinematicStage::PhaseSpell;
        ChangeState(GetSpellingState());
        return;
    }
    ChangeState(GetIdlingState());
}

/// Releases gameplay only after the complete forced spell cycle has finished.
void Boss::CompleteSpellingState() {
    if (cinematicStage == BossCinematicStage::PhaseSpell) {
        cinematicStage = BossCinematicStage::None;
    }
    ChangeState(GetIdlingState());
}

/// Frames the boss tightly on introduction and widens to include the complete
/// maximum summon radius during a phase transition.
Rectangle Boss::GetCinematicCameraBounds() const {
    float width = cinematicStage == BossCinematicStage::Introduction
        ? BOSS_INTRO_CAMERA_WIDTH
        : BOSS_PHASE_CAMERA_HALF_EXTENT * 2.0f;
    float height = cinematicStage == BossCinematicStage::Introduction
        ? BOSS_INTRO_CAMERA_HEIGHT
        : BOSS_PHASE_CAMERA_HALF_EXTENT * 2.0f;
    return {
        position.x - width * 0.5f,
        position.y - height * 0.5f,
        width,
        height
    };
}

/// Emits the circular projectile patterns associated with a completed boss stomp.
void Boss::FireStompProjectiles() {
    Vector2 origin = GetStompFootWorldPosition();
    GameManager& gameManager = GameManager::GetInstance();
    float speedScale = GetPhase() == BossPhase::Phase3
        ? BOSS_PHASE_THREE_STOMP_BULLET_SPEED_SCALE
        : 1.0f;

    if (stompDroneBulletTexture.id != 0) {
        for (int index = 0;
             index < BOSS_STOMP_DRONE_BULLET_COUNT;
             ++index) {
            float angle = 2.0f * PI * index /
                BOSS_STOMP_DRONE_BULLET_COUNT;
            Vector2 direction = { std::cos(angle), std::sin(angle) };
            Vector2 spawnPosition = Vector2Add(
                origin,
                Vector2Scale(
                    direction,
                    BOSS_STOMP_PROJECTILE_SPAWN_RADIUS
                )
            );
            gameManager.AddProjectile(std::make_unique<DroneBullet>(
                spawnPosition,
                direction,
                STOMP_DRONE_BULLET_INITIAL_SPEED * speedScale,
                STOMP_DRONE_BULLET_MINIMUM_SPEED * speedScale,
                STOMP_DRONE_BULLET_DRAG * speedScale,
                STOMP_DRONE_BULLET_LIFETIME,
                STOMP_DRONE_BULLET_RADIUS,
                STOMP_DRONE_BULLET_DAMAGE,
                stompDroneBulletTexture,
                true
            ));
        }
    }

    if (stompKnightBulletTexture.id != 0) {
        for (int index = 0;
             index < BOSS_STOMP_KNIGHT_BULLET_COUNT;
             ++index) {
            float angle = 2.0f * PI * index /
                BOSS_STOMP_KNIGHT_BULLET_COUNT;
            Vector2 direction = { std::cos(angle), std::sin(angle) };
            Vector2 spawnPosition = Vector2Add(
                origin,
                Vector2Scale(
                    direction,
                    BOSS_STOMP_PROJECTILE_SPAWN_RADIUS
                )
            );
            gameManager.AddProjectile(std::make_unique<Projectile>(
                spawnPosition,
                Vector2Scale(
                    direction,
                    STOMP_KNIGHT_BULLET_SPEED * speedScale
                ),
                STOMP_KNIGHT_BULLET_LIFETIME,
                STOMP_KNIGHT_BULLET_DAMAGE,
                stompKnightBulletTexture,
                true,
                STOMP_KNIGHT_BULLET_RADIUS
            ));
        }
    }
}

/// Creates one homing fire-punch projectile from the animated hand's muzzle position.
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

    gameManager.AddProjectile(std::make_unique<BossFirePunchProjectile>(
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

/// Resets animation cycle.
void Boss::ResetAnimationCycle() {
    currentRunFrame = 0;
    runFrameTime = 0.0f;
}
