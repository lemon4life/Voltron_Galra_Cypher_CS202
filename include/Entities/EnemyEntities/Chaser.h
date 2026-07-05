#pragma once
#include "AI/EnemyState.h"
#include "Entities/Enemy.h"
#include "Core/IEnemyObserver.h"
#include "raylib.h"
#include <vector>

const int MAX_HEALTH = 50;
const float BASE_SPEED = 100.f;
const int BASE_DAMAGE = 15;
const float BASE_ATTACK_COOLDOWN = 0.1f;

class EnemyChaser : public Enemy {
private:
    float width = 20.f;
    float height = 20.f;

    EnemyIdleState idleState;
    EnemyChaserChaseState chaseState;

    bool usePathFinding = false;
    Vector2 targetPosition = {-1.f,-1.f};
public:
    EnemyChaser(Vector2 pos, Player* target);
    ~EnemyChaser() override;

    Rectangle GetBoundingBox() const override;

    void SetTargetPosition(Vector2 target) override { targetPosition = target; }
    Vector2 GetTargetPosition() const override { return targetPosition; }
    bool HasTargetPosition() const override { return targetPosition.x >= 0.0f && targetPosition.y >= 0.0f; }

    void Update(float deltaTime) override;
    void Draw() override;

    void StartPathFinding();
    void EndPathFinding();

    IEnemyState* GetIdleState() override { return &idleState; }
    IEnemyState* GetChaseState() override { return &chaseState; }

    IEnemyState* GetChaserIdleState() { return &idleState; }
    IEnemyState* GetChaserChaseState() { return &chaseState; }
};
