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
    /// Creates a EnemyIdleState instance from the supplied configuration.
    explicit EnemyIdleState(float spotDistance);

    /// Prepares this state when it becomes active.
    void Enter(Enemy* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(Enemy* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Enemy* enemy) override;
};

class EnemyDazeState : public IEnemyState {
private:
    float dTimer = 0.0f;

public:
    /// Prepares this state when it becomes active.
    void Enter(Enemy* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(Enemy* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Enemy* enemy) override;

    /// Returns the current remaining time.
    float GetRemainingTime() const { return dTimer; }
};

/* 
    Chasing behavior for enemy "Chaser":
*/

class EnemyChaserChaseState : public ITypedEnemyState<EnemyChaser> {
public:
    /// Prepares this state when it becomes active.
    void Enter(EnemyChaser* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(EnemyChaser* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(EnemyChaser* enemy) override;
};

class EnemyChaserDamageState : public ITypedEnemyState<EnemyChaser> {
private:
    float dTimer = 0.0f;
    float remainingChargeDistance = 0.0f;
    Vector2 chargeDirection = { 0.0f, 0.0f };
    bool attackResolved = false;

public:
    /// Prepares this state when it becomes active.
    void Enter(EnemyChaser* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(EnemyChaser* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(EnemyChaser* enemy) override;
};

/*
    Chase, preparation, and lunging behavior for enemy "Diver":
*/

class EnemyDiverChaseState : public ITypedEnemyState<EnemyDiver> {
public:
    /// Prepares this state when it becomes active.
    void Enter(EnemyDiver* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(EnemyDiver* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(EnemyDiver* enemy) override;
};

class EnemyDiverReadyState : public ITypedEnemyState<EnemyDiver> {
private:
    float dTimer = 0.0f;

public:
    /// Prepares this state when it becomes active.
    void Enter(EnemyDiver* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(EnemyDiver* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(EnemyDiver* enemy) override;
};

class EnemyDiverLungingState : public ITypedEnemyState<EnemyDiver> {
private:
    float dTimer = 0.0f;
    bool isWaitingToChase = false;
    bool hasDamagedPlayer = false;

    /// Begins recovery.
    void BeginRecovery(EnemyDiver* enemy);

public:
    /// Prepares this state when it becomes active.
    void Enter(EnemyDiver* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(EnemyDiver* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(EnemyDiver* enemy) override;
};


/*
    Chasing & shooting behavior for enemy "Ranger":
*/

class EnemyRangeChaseState : public ITypedEnemyState<EnemyRange> {
public:
    /// Prepares this state when it becomes active.
    void Enter(EnemyRange* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(EnemyRange* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(EnemyRange* enemy) override;
};

class EnemyRangeShootingState : public ITypedEnemyState<EnemyRange> {
private:
    /// Attempts to fire projectile.
    bool TryFireProjectile(EnemyRange* enemy, Vector2 targetPosition);

public:
    /// Prepares this state when it becomes active.
    void Enter(EnemyRange* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(EnemyRange* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
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
    /// Prepares this state when it becomes active.
    void Enter(Boss* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(Boss* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Boss* enemy) override;
};

class BossSpellingState : public ITypedEnemyState<Boss> {
private:
    float elapsedTime = 0.0f;
    float spellDuration = 0.0f;
    float nextSummonCheck = 0.0f;
    float cloneSummonRetryTimer = 0.0f;
    int demonsSummoned = 0;

public:
    /// Prepares this state when it becomes active.
    void Enter(Boss* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(Boss* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
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
    /// Prepares this state when it becomes active.
    void Enter(Boss* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(Boss* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Boss* enemy) override;

    /// Returns the current phase.
    Phase GetPhase() const { return phase; }
    /// Returns the current frame index.
    int GetFrameIndex() const { return frameIndex; }
    /// Returns the current completed punches.
    int GetCompletedPunches() const { return completedPunches; }
};

class BossStompingState : public ITypedEnemyState<Boss> {
private:
    float frameTimer = 0.0f;
    int frameIndex = 0;
    int completedStomps = 0;
    int stompsForState = 0;

public:
    /// Prepares this state when it becomes active.
    void Enter(Boss* enemy) override;
    /// Advances this component's state for the current frame.
    void Update(Boss* enemy, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Boss* enemy) override;

    /// Returns the current frame index.
    int GetFrameIndex() const { return frameIndex; }
    /// Returns the current completed stomps.
    int GetCompletedStomps() const { return completedStomps; }
};
