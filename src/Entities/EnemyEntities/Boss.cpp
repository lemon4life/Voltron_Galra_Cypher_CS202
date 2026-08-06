#include "Entities/EnemyEntities/Boss.h"

#include "Core/Manager/AssetManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr int BOSS_MAX_HEALTH = 500;
    constexpr float BOSS_SPEED = 75.0f;
    constexpr int BOSS_DAMAGE = 25;
    constexpr float BOSS_ATTACK_COOLDOWN = 0.8f;
    constexpr float BOSS_SIGHT_DISTANCE = 900.0f;
    constexpr float BOSS_OFF_SIGHT_DISTANCE = 1200.0f;

    constexpr int BOSS_FRAME_COUNT = 4;
    constexpr float BOSS_RENDER_WIDTH = 128.0f;
    constexpr float BOSS_RENDER_HEIGHT = 128.0f;
    constexpr float BOSS_IDLE_FRAME_DURATION = 0.18f;
    constexpr float BOSS_RUN_FRAME_DURATION = 0.11f;
    constexpr float BOSS_KNOCKBACK_RESISTANCE = 1.0f;

    // Across all frames, the non-transparent artwork occupies x=25..120 and
    // y=4..127: approximately 96x124 pixels. The directional origins center
    // that asymmetric visible region rather than the transparent 128x128 cell.
    constexpr Vector2 BOSS_SIZE = { 96.0f, 124.0f };
    constexpr float BOSS_UNFLIPPED_ORIGIN_X = 72.5f;
    constexpr float BOSS_FLIPPED_ORIGIN_X = 54.5f;
    constexpr float BOSS_ORIGIN_Y = 66.0f;
    constexpr EnemyCollisionProfile BOSS_COLLISION_PROFILE = {
        { 56.0f, 18.0f },
        { 0.0f, 53.0f }
    };

    constexpr float BOSS_HEALTH_BAR_WIDTH = 96.0f;
    constexpr float BOSS_HEALTH_BAR_HEIGHT = 6.0f;
    constexpr float BOSS_HEALTH_BAR_GAP = 8.0f;
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
    bossSkillCooldown = 2.0f;
    burstCount = 0;
    burstTimer = 0.0f;

    enemyType = EnemyType::BOSS;
    size = BOSS_SIZE;
    SetCollisionProfile(BOSS_COLLISION_PROFILE);
    SetKnockbackResistance(BOSS_KNOCKBACK_RESISTANCE);

    idleState = std::make_unique<EnemyIdleState>(BOSS_SIGHT_DISTANCE);
    chaseState = std::make_unique<BossChaseState>();
    rangeState = std::make_unique<BossRangedAttackState>();

    SetEnemySprites(AssetManager::GetInstance().GetBossSprites());

    ChangeState(GetIdleState());
}

Boss::~Boss() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void Boss::Update(float deltaTime) {
    UpdateKnockback(deltaTime);
    if (currentState) {
        currentState->Update(this, deltaTime);
    }

    if (health <= 0) return;

    if (targetTeam && targetTeam->GetActivePaladin()) {
        facingLeft = targetTeam->GetActivePaladin()->GetPosition().x <
            position.x;
    }

    float frameDuration = currentState == chaseState.get()
        ? BOSS_RUN_FRAME_DURATION
        : BOSS_IDLE_FRAME_DURATION;
    runFrameTime += deltaTime;
    while (runFrameTime >= frameDuration) {
        runFrameTime -= frameDuration;
        currentRunFrame = (currentRunFrame + 1) % BOSS_FRAME_COUNT;
    }
}

void Boss::Draw() {
    Texture2D texture = currentState == chaseState.get()
        ? sprites.run
        : sprites.idle;
    // The supplied Boss artwork faces left in its unflipped orientation.
    bool flipSprite = !facingLeft;

    if (texture.id != 0 && texture.width >= BOSS_FRAME_COUNT) {
        float frameWidth = (float)texture.width / BOSS_FRAME_COUNT;
        float frameHeight = (float)texture.height;
        Rectangle source = {
            currentRunFrame * frameWidth,
            0.0f,
            flipSprite ? -frameWidth : frameWidth,
            frameHeight
        };
        Rectangle destination = {
            std::round(position.x),
            std::round(position.y),
            BOSS_RENDER_WIDTH,
            BOSS_RENDER_HEIGHT
        };
        Vector2 origin = {
            flipSprite
                ? BOSS_FLIPPED_ORIGIN_X
                : BOSS_UNFLIPPED_ORIGIN_X,
            BOSS_ORIGIN_Y
        };
        DrawTexturePro(
            texture,
            source,
            destination,
            origin,
            0.0f,
            WHITE
        );
    } else {
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

    DrawPathDebug();
}

bool Boss::IsBeyondDisengageDistance(Vector2 targetPosition) const {
    return Vector2Distance(position, targetPosition) > BOSS_OFF_SIGHT_DISTANCE;
}
