#pragma once

#include "Entities/Enemy.h"

enum class BossPhase {
    Phase1,
    Phase2,
    Phase3
};

/// Identifies which scripted boss sequence currently owns gameplay and camera control.
enum class BossCinematicStage {
    None,
    Introduction,
    PhaseStomps,
    PhaseSpell
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
    BossCinematicStage cinematicStage = BossCinematicStage::Introduction;
    bool introPlayed = false;

    /// Returns the current stomp foot world position.
    Vector2 GetStompFootWorldPosition() const;
    /// Detects boss health thresholds and starts each one-time phase transition.
    void EvaluatePhaseTransitions();
    /// Starts the non-interactive stomp-and-spell ceremony for a new phase.
    void StartPhaseCinematic();
    /// Attempts to summon boss clone.
    bool TrySummonBossClone(int cloneHealth, BossPhase clonePhase);
    /// Returns the current body tint.
    Color GetBodyTint() const;

public:
    /// Creates a Boss instance from the supplied configuration.
    Boss(
        Vector2 pos,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess
    );
    /// Releases resources owned by this Boss instance.
    ~Boss() override;

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    /// Applies incoming damage after this object handles defenses and state-specific rules.
    void TakeDamage(int amount) override;

    /// Returns the current idling state.
    BossIdlingState* GetIdlingState() {
        return static_cast<BossIdlingState*>(idleState.get());
    }
    /// Returns the current spelling state.
    BossSpellingState* GetSpellingState() { return spellingState.get(); }
    /// Returns the current punch state.
    BossPunchState* GetPunchState() { return punchState.get(); }
    /// Returns the current stomping state.
    BossStompingState* GetStompingState() { return stompingState.get(); }
    /// Reports whether the spelling condition is satisfied.
    bool IsSpelling() const { return currentState == spellingState.get(); }
    /// Reports whether a normal offense roll may select the spell state.
    bool CanSelectRandomSpell() const;
    /// Reports whether the punching condition is satisfied.
    bool IsPunching() const { return currentState == punchState.get(); }
    /// Reports whether the stomping condition is satisfied.
    bool IsStomping() const { return currentState == stompingState.get(); }
    /// Reports whether the in offensive state condition is satisfied.
    bool IsInOffensiveState() const {
        return IsSpelling() || IsPunching() || IsStomping();
    }
    /// Reports whether the clone condition is satisfied.
    bool IsClone() const { return cloneBoss; }
    /// Returns the current phase.
    BossPhase GetPhase() const;
    /// Configures as clone.
    void ConfigureAsClone(int cloneHealth, BossPhase clonePhase);
    /// Reports whether this component has pending phase clone summons.
    bool HasPendingPhaseCloneSummons() const {
        return phaseOneClonePending || phaseTwoClonePending;
    }
    /// Attempts to summon pending phase clones.
    void TrySummonPendingPhaseClones();
    /// Returns the current idle minimum milliseconds.
    int GetIdleMinimumMilliseconds() const;
    /// Returns the current idle maximum milliseconds.
    int GetIdleMaximumMilliseconds() const;
    /// Returns the current idle movement speed scale.
    float GetIdleMovementSpeedScale() const;
    /// Returns the current stomps per state.
    int GetStompsPerState() const;
    /// Returns the stomp count selected for the currently active state cycle.
    int GetCurrentStompCount() const;
    /// Returns the current punches per state.
    int GetPunchesPerState() const;
    /// Returns the current spell summon interval.
    float GetSpellSummonInterval() const;
    /// Returns the current spell summon chance percent.
    int GetSpellSummonChancePercent() const;
    /// Attempts to summon random enemy.
    bool TrySummonRandomEnemy(int& demonsSummonedThisSpell);
    /// Spawns stomp smoke.
    void SpawnStompSmoke();
    /// Applies audiovisual impact feedback and optional combat projectiles.
    void HandleStompImpact();
    /// Selects the state following a completed stomp sequence.
    void CompleteStompingState();
    /// Selects the state following a completed spell sequence.
    void CompleteSpellingState();
    /// Emits the circular projectile patterns associated with a completed boss stomp.
    void FireStompProjectiles();
    /// Creates one homing fire-punch projectile from the animated hand's muzzle position.
    void FirePunchProjectile(
        float bulletSpeed,
        float changeAngleDegreesPerSecond
    );
    /// Resets animation cycle.
    void ResetAnimationCycle();

    /// Reports whether this boss currently owns a gameplay cinematic.
    bool IsCinematicActive() const {
        return cinematicStage != BossCinematicStage::None;
    }
    /// Reports whether summoned enemies may advance their spawn-only animation.
    bool AllowsCinematicSpawnAnimations() const {
        return cinematicStage == BossCinematicStage::PhaseSpell;
    }
    /// Returns the world area which the cinematic camera must keep visible.
    Rectangle GetCinematicCameraBounds() const;
};
