#include "Entities/EnemyEntities/Boss.h"

namespace {
    constexpr int BOSS_MAX_HEALTH = 500;
    constexpr float BOSS_SPEED = 75.0f;
    constexpr int BOSS_DAMAGE = 25;
    constexpr float BOSS_ATTACK_COOLDOWN = 0.8f;
    constexpr float BOSS_SIGHT_DISTANCE = 900.0f;
    constexpr float BOSS_OFF_SIGHT_DISTANCE = 1200.0f;
    constexpr Vector2 BOSS_SIZE = Vector2{64.f,72.f};
}

Boss::Boss(Vector2 pos, TeamManager* targetTeam)
    : Enemy(pos, targetTeam, BOSS_MAX_HEALTH, BOSS_SPEED, BOSS_DAMAGE, BOSS_ATTACK_COOLDOWN)
{
    bossSkillCooldown = 2.0f;
    burstCount = 0;
    burstTimer = 0.0f;

    enemyType = EnemyType::BOSS;
    size = BOSS_SIZE;

    idleState = std::make_unique<EnemyIdleState>();
    chaseState = std::make_unique<BossChaseState>();
    rangeState = std::make_unique<BossRangedAttackState>();


    idleState->UpdateDistance(BOSS_SIGHT_DISTANCE);
    chaseState->UpdateDistance(BOSS_OFF_SIGHT_DISTANCE);
    rangeState->UpdateDistance(BOSS_SIGHT_DISTANCE);

    ChangeState(GetIdleState());
}

Boss::~Boss() {
    if (currentState) {
        currentState->Exit(this);
    }
    currentState = nullptr;
}

void Boss::Update(float deltaTime) {
    if (currentState) {
        currentState->Update(this, deltaTime);
    }
}

void Boss::Draw() {
    Color col = ORANGE;
    DrawRectangleRec(GetBoundingBox(), col);
    
    // Draw Health Bar
    float hpPercent = (float)health / maxHealth;
    DrawRectangle(position.x - size.x/2.f, position.y - size.y/2.f, size.x * hpPercent, 4, RED);
}
