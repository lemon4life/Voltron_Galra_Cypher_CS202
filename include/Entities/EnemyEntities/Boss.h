#pragma once

#include "Entities/Enemy.h"

enum class BossPhase {
    Phase1,
    Phase2,
    Phase3
};

class Boss : public Enemy {
private:
    std::unique_ptr<BossSpellingState> spellingState;
    std::unique_ptr<BossPunchState> punchState;
    std::unique_ptr<BossStompingState> stompingState;
    Texture2D spellTexture = { 0 };
    Texture2D punchReadyTexture = { 0 };
    Texture2D punchBodyTexture = { 0 };
    Texture2D punchHandTexture = { 0 };
    Texture2D firePunchTexture = { 0 };
    Texture2D stompTexture = { 0 };
    Texture2D stompSmokeTexture = { 0 };
    Texture2D stompDroneBulletTexture = { 0 };
    Texture2D stompKnightBulletTexture = { 0 };

    bool phaseLocked = false;
    BossPhase lockedPhase = BossPhase::Phase1;
    bool cloneBoss = false;
    bool phaseTwoTransitionTriggered = false;
    bool phaseThreeTransitionTriggered = false;
    bool phaseOneClonePending = false;
    bool phaseTwoClonePending = false;

    Vector2 GetStompFootWorldPosition() const;
    void EvaluatePhaseTransitions();
    void RestartOrEnterSpellingState();
    bool TrySummonBossClone(int cloneHealth, BossPhase clonePhase);
    Color GetBodyTint() const;

public:
    Boss(
        Vector2 pos,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess
    );
    ~Boss() override;

    void Update(float deltaTime) override;
    void Draw() override;
    void TakeDamage(int amount) override;

    BossIdlingState* GetIdlingState() {
        return static_cast<BossIdlingState*>(idleState.get());
    }
    BossSpellingState* GetSpellingState() { return spellingState.get(); }
    BossPunchState* GetPunchState() { return punchState.get(); }
    BossStompingState* GetStompingState() { return stompingState.get(); }
    bool IsSpelling() const { return currentState == spellingState.get(); }
    bool IsPunching() const { return currentState == punchState.get(); }
    bool IsStomping() const { return currentState == stompingState.get(); }
    bool IsInOffensiveState() const {
        return IsSpelling() || IsPunching() || IsStomping();
    }
    bool IsClone() const { return cloneBoss; }
    BossPhase GetPhase() const;
    void ConfigureAsClone(int cloneHealth, BossPhase clonePhase);
    bool HasPendingPhaseCloneSummons() const {
        return phaseOneClonePending || phaseTwoClonePending;
    }
    void TrySummonPendingPhaseClones();
    int GetIdleMinimumMilliseconds() const;
    int GetIdleMaximumMilliseconds() const;
    float GetIdleMovementSpeedScale() const;
    int GetStompsPerState() const;
    int GetPunchesPerState() const;
    float GetSpellSummonInterval() const;
    int GetSpellSummonChancePercent() const;
    bool TrySummonRandomEnemy(int& demonsSummonedThisSpell);
    void SpawnStompSmoke();
    void FireStompProjectiles();
    void FirePunchProjectile(
        float bulletSpeed,
        float changeAngleDegreesPerSecond
    );
    void ResetAnimationCycle();
};
