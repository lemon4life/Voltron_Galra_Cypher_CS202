#pragma once
#include "IEnemyState.h"
#include "raylib.h"

class EnemyChaser;
class EnemyDiver;
class EnemyRange;
class Boss;
class Paladin;

/* 
    A Default Behaviour for Enemies
*/

class EnemyIdleState : public IEnemyState {
private:
    const float spotDistance;
public:
    explicit EnemyIdleState(float spotDistance);

    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
};

class EnemyDazeState : public IEnemyState {
private:
    float dTimer = 0.0f;

public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    float GetRemainingTime() const { return dTimer; }
};

/* 
    Chasing behavior for enemy "Chaser":
*/

class EnemyChaserChaseState : public ITypedEnemyState<EnemyChaser> {
public:
    void Enter(EnemyChaser* enemy) override;
    void Update(EnemyChaser* enemy, float deltaTime) override;
    void Exit(EnemyChaser* enemy) override;
};

class EnemyChaserDamageState : public ITypedEnemyState<EnemyChaser> {
private:
    float dTimer = 0.0f;
    float remainingChargeDistance = 0.0f;
    Vector2 chargeDirection = { 0.0f, 0.0f };
    bool attackResolved = false;

public:
    void Enter(EnemyChaser* enemy) override;
    void Update(EnemyChaser* enemy, float deltaTime) override;
    void Exit(EnemyChaser* enemy) override;
};

/*
    Chase, preparation, and lunging behavior for enemy "Diver":
*/

class EnemyDiverChaseState : public ITypedEnemyState<EnemyDiver> {
public:
    void Enter(EnemyDiver* enemy) override;
    void Update(EnemyDiver* enemy, float deltaTime) override;
    void Exit(EnemyDiver* enemy) override;
};

class EnemyDiverReadyState : public ITypedEnemyState<EnemyDiver> {
private:
    float dTimer = 0.0f;

public:
    void Enter(EnemyDiver* enemy) override;
    void Update(EnemyDiver* enemy, float deltaTime) override;
    void Exit(EnemyDiver* enemy) override;
};

class EnemyDiverLungingState : public ITypedEnemyState<EnemyDiver> {
private:
    float dTimer = 0.0f;
    Vector2 lockedDirection = { 0.0f, 0.0f };
    bool isWaitingToChase = false;
    bool hasDamagedPlayer = false;

    void BeginRecovery(EnemyDiver* enemy);

public:
    void Enter(EnemyDiver* enemy) override;
    void Update(EnemyDiver* enemy, float deltaTime) override;
    void Exit(EnemyDiver* enemy) override;
};


/*
    Chasing & shooting behavior for enemy "Ranger":
*/

class EnemyRangeChaseState : public ITypedEnemyState<EnemyRange> {
public:
    void Enter(EnemyRange* enemy) override;
    void Update(EnemyRange* enemy, float deltaTime) override;
    void Exit(EnemyRange* enemy) override;
};

class EnemyRangeShootingState : public ITypedEnemyState<EnemyRange> {
private:
    Vector2 previousPlayerPosition = { 0.0f, 0.0f };
    Vector2 estimatedPlayerVelocity = { 0.0f, 0.0f };
    bool hasPreviousPlayerPosition = false;

    Vector2 PredictTargetPosition(EnemyRange* enemy, Paladin* player, float deltaTime);
    void FireProjectile(EnemyRange* enemy, Vector2 targetPosition);

public:
    void Enter(EnemyRange* enemy) override;
    void Update(EnemyRange* enemy, float deltaTime) override;
    void Exit(EnemyRange* enemy) override;
};

/* 
    Boss Ranged Attack State
*/

class BossChaseState : public ITypedEnemyState<Boss> {
public:
    void Enter(Boss* enemy) override;
    void Update(Boss* enemy, float deltaTime) override;
    void Exit(Boss* enemy) override;
};

class BossRangedAttackState : public IEnemyState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;
};
