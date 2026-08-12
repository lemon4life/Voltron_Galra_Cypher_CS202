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
    constexpr int BOSS_IDLE_FRAME_COUNT = 10;
    constexpr int BOSS_RUN_FRAME_COUNT = 6;
    constexpr float BOSS_FRAME_WIDTH = 64.0f;
    constexpr float BOSS_FRAME_HEIGHT = 72.0f;
    constexpr float BOSS_IDLE_FRAME_DURATION = 0.18f;
    constexpr float BOSS_RUN_FRAME_DURATION = 0.11f;
    constexpr float BOSS_KNOCKBACK_RESISTANCE = 1.0f;

    // Across the ten idle and six running frames, visible pixels occupy the
    // combined x=8..54 and y=3..71 bounds inside each 64x72 cell.
    constexpr Vector2 BOSS_SIZE = { 47.0f, 69.0f };
    constexpr Vector2 BOSS_DRAW_ORIGIN = { 31.5f, 37.5f };

    // The bottom twelve source rows contain the stable foot silhouette. Its
    // center is 28.5 pixels below the recalculated visible-body center.
    constexpr EnemyCollisionProfile BOSS_COLLISION_PROFILE = {
        { 32.0f, 12.0f },
        { 0.0f, 28.5f }
    };

    constexpr float BOSS_HEALTH_BAR_WIDTH = BOSS_FRAME_WIDTH;
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
    enemyType = EnemyType::BOSS;
    size = BOSS_SIZE;
    SetCollisionProfile(BOSS_COLLISION_PROFILE);
    SetKnockbackResistance(BOSS_KNOCKBACK_RESISTANCE);

    idleState = std::make_unique<BossIdlingState>();
    chaseState = std::make_unique<BossChaseState>();

    SetEnemySprites(AssetManager::GetInstance().GetBossSprites());

    ChangeState(GetIdlingState());
}

Boss::~Boss() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void Boss::Update(float deltaTime) {
    UpdateKnockback(deltaTime);
    
    if (statusComponent.Update(deltaTime, this)) {
        return;
    }
    
    if (currentState) {
        currentState->Update(this, deltaTime);
    }

    if (health <= 0) return;

    if (targetTeam && targetTeam->GetActivePaladin()) {
        facingLeft = targetTeam->GetActivePaladin()->GetPosition().x <
            position.x;
    }

    bool useRunAnimation = currentState == chaseState.get();
    int frameCount = useRunAnimation
        ? BOSS_RUN_FRAME_COUNT
        : BOSS_IDLE_FRAME_COUNT;
    float frameDuration = useRunAnimation
        ? BOSS_RUN_FRAME_DURATION
        : BOSS_IDLE_FRAME_DURATION;
    currentRunFrame %= frameCount;
    runFrameTime += deltaTime;
    while (runFrameTime >= frameDuration) {
        runFrameTime -= frameDuration;
        currentRunFrame = (currentRunFrame + 1) % frameCount;
    }
}

void Boss::Draw() {
    bool useRunAnimation = currentState == chaseState.get();
    int frameCount = useRunAnimation
        ? BOSS_RUN_FRAME_COUNT
        : BOSS_IDLE_FRAME_COUNT;
    int frameIndex = currentRunFrame % frameCount;
    Texture2D texture = useRunAnimation
        ? sprites.run
        : sprites.idle;
    // Invert the sheet's authored direction to match the Boss target facing.
    bool flipSprite = facingLeft;

    if (texture.id != 0 &&
        texture.width >= (int)(BOSS_FRAME_WIDTH * frameCount) &&
        texture.height >= (int)BOSS_FRAME_HEIGHT) {
        Rectangle source = {
            frameIndex * BOSS_FRAME_WIDTH,
            0.0f,
            flipSprite ? -BOSS_FRAME_WIDTH : BOSS_FRAME_WIDTH,
            BOSS_FRAME_HEIGHT
        };
        Rectangle destination = {
            std::round(position.x),
            std::round(position.y),
            BOSS_FRAME_WIDTH,
            BOSS_FRAME_HEIGHT
        };
        DrawTexturePro(
            texture,
            source,
            destination,
            BOSS_DRAW_ORIGIN,
            0.0f,
            statusComponent.GetStatusTint()
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
