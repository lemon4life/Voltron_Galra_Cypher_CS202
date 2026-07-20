#pragma once
#include "AI/EnemyState.h"
#include "Entities/Enemy.h"
#include "Core/EnemyPath.h"

#include <vector>

static const int MAX_HEALTH = 80;
static const float BASE_SPEED = 170.f;
static const int BASE_DAMAGE = 15;
static const float BASE_ATTACK_COOLDOWN = 0.5f;
static const float SIGHT = 40000.f;

static const float WIDTH = 20.f;
static const float HEIGHT = 20.f;

class EnemyChaser : public Enemy, public EnemyPathFinding {
private:
public:
    EnemyChaser(
        Vector2 pos,
        TeamManager* targetTeam,
        IEntityRemovalAccess* removalAccess,
        IEnemyPathAccess* pathAccess
    );
    ~EnemyChaser() override;

    void Update(float deltaTime) override;
    void Draw() override;

    void StartPathFinding();
    void EndPathFinding();
};
