#include "Entities/EnemyEntities/Chaser.h"
#include "Core/AudioManager.h"
#include "Core/EnemyPathManager.h"
#include "Core/GameManager.h"

#include <iostream>

EnemyChaser::EnemyChaser(Vector2 pos, Player* target)
    : Enemy(pos, target, MAX_HEALTH, BASE_SPEED, BASE_DAMAGE, BASE_ATTACK_COOLDOWN)
{
    idleState.updateSpotDistance(500.f);
    chaseState.updateSightDistance(500.f);
    currentState = &idleState;
    currentState->Enter(this);
}

EnemyChaser::~EnemyChaser() {
    EndPathFinding();
    currentState = nullptr;
}

void EnemyChaser::Update(float deltaTime) {
    if (currentState) {
        currentState->Update(this, deltaTime);
    }

    std::cout << "TargetPos: " <<
        targetPosition.x << " " << targetPosition.y <<
        " In Path: " << usePathFinding << std::endl;
}

void EnemyChaser::Draw() {

    // Different state gives different color
    if (currentState == &chaseState) {
        DrawRectangleRec(GetBoundingBox(), MAROON);
    }
    else {
        DrawRectangleRec(GetBoundingBox(), LIME);
    }

    float hpPercent = (float) health / (float) maxHealth;
    DrawRectangle(position.x - width / 2.f, position.y - 20, width * hpPercent, 4, RED);
}

Rectangle EnemyChaser::GetBoundingBox() const {
    // bounding box centered on position
    return { position.x - width / 2.f, position.y - height / 2.f, width, height };
}

void EnemyChaser::StartPathFinding() {
    if (usePathFinding) return;
    usePathFinding = true;

    NotifyEnemyPathFind();
}


void EnemyChaser::EndPathFinding() {
    if (!usePathFinding) return;
    usePathFinding = false;
    targetPosition = { -1.0f, -1.0f };

    NotifyEnemyPathFindEnded();
}
