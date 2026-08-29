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
    bool TryFireProjectile(EnemyRange* enemy, Vector2 targetPosition);

public:
    void Enter(EnemyRange* enemy) override;
    void Update(EnemyRange* enemy, float deltaTime) override;
    void Exit(EnemyRange* enemy) override;
};

/*
    Boss idle and randomized offense behaviors
*/

class BossIdlingState : public ITypedEnemyState<Boss> {
private:
    enum class Stage {
        InitialIdle,
        MoveToPlayer,
        FinalIdle
    };

    Stage stage = Stage::InitialIdle;
    float stageTimeRemaining = 0.0f;

public:
    void Enter(Boss* enemy) override;
    void Update(Boss* enemy, float deltaTime) override;
    void Exit(Boss* enemy) override;
};

class BossSpellingState : public ITypedEnemyState<Boss> {
private:
    float elapsedTime = 0.0f;
    float spellDuration = 0.0f;
    float nextSummonCheck = 0.0f;
    int demonsSummoned = 0;

public:
    void Enter(Boss* enemy) override;
    void Update(Boss* enemy, float deltaTime) override;
    void Exit(Boss* enemy) override;
};

class BossPunchState : public ITypedEnemyState<Boss> {
public:
    enum class Phase {
        Ready,
        Punch
    };

private:
    Phase phase = Phase::Ready;
    float frameTimer = 0.0f;
    int frameIndex = 0;
    int completedPunches = 0;
    int punchesForState = 0;
    float bulletSpeed = 250.0f;
    float changeAngleDegreesPerSecond = 30.0f;

public:
    void Enter(Boss* enemy) override;
    void Update(Boss* enemy, float deltaTime) override;
    void Exit(Boss* enemy) override;

    Phase GetPhase() const { return phase; }
    int GetFrameIndex() const { return frameIndex; }
    int GetCompletedPunches() const { return completedPunches; }
};

class BossStompingState : public ITypedEnemyState<Boss> {
private:
    float frameTimer = 0.0f;
    int frameIndex = 0;
    int completedStomps = 0;
    int stompsForState = 0;

public:
    void Enter(Boss* enemy) override;
    void Update(Boss* enemy, float deltaTime) override;
    void Exit(Boss* enemy) override;

    int GetFrameIndex() const { return frameIndex; }
    int GetCompletedStomps() const { return completedStomps; }
};
