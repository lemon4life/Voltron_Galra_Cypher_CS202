#pragma once

#include "Core/EnemyPath.h"
#include "Entities/Enemy.h"

#include <memory>

class EnemyRangeShootingState;

constexpr int RANGE_MAX_HEALTH = 70;
constexpr float RANGE_SPEED = 120.0f;
constexpr int RANGE_DAMAGE = 12;
constexpr float RANGE_ATTACK_COOLDOWN = 1.0f;
constexpr float RANGE_DETECTION_DISTANCE = 700.0f;
constexpr float RANGE_DISENGAGE_DISTANCE = 900.0f;
constexpr float RANGE_SHOOTING_DISTANCE = 200.0f;
constexpr float RANGE_PROJECTILE_SPEED = 320.0f;
constexpr float RANGE_PROJECTILE_LIFETIME = 2.0f;
constexpr float RANGE_PROJECTILE_RADIUS = 5.0f;
constexpr float MAX_PREDICTION_TIME = 1.0f;
constexpr Vector2 RANGE_SIZE = { 24.0f, 24.0f };

class EnemyRange : public Enemy, public EnemyPathFinding {
private:
    std::unique_ptr<EnemyRangeShootingState> shootingState;

public:
    EnemyRange(Vector2 position, TeamManager* targetTeam);
    ~EnemyRange() override;

    void Update(float deltaTime) override;
    void Draw() override;

    EnemyRangeShootingState* GetShootingState();

    bool IsWithinShootingDistance(Vector2 targetPosition) const;
    bool IsBeyondDisengageDistance(Vector2 targetPosition) const;
    float GetProjectileSpeed() const;
    float GetProjectileLifetime() const;
    float GetProjectileRadius() const;
    float GetMaxPredictionTime() const;

    void StartPathFinding();
    void EndPathFinding();
};
