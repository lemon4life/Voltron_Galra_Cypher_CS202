#pragma once

#include "Entities/Enemy.h"

#include <memory>

class DemonTHAAggroState;
class DemonTHAWanderIdleState;
class DemonTHAWanderMoveState;
class Paladin;

class DemonTHA : public Enemy {
private:
    enum class BodyAnimation {
        Idle,
        Run,
        Shooting
    };

    struct GunPose {
        Vector2 anchorWorld = { 0.0f, 0.0f };
        Vector2 drawOrigin = { 0.0f, 0.0f };
        Vector2 shotOriginWorld = { 0.0f, 0.0f };
        float angleDegrees = 0.0f;
        bool flipSprite = false;
    };

    std::unique_ptr<DemonTHAWanderIdleState> wanderIdleState;
    std::unique_ptr<DemonTHAWanderMoveState> wanderMoveState;
    std::unique_ptr<DemonTHAAggroState> aggroState;
    ILevelLineOfSightQuery& lineOfSightQuery;

    Texture2D idleTexture = { 0 };
    Texture2D runTexture = { 0 };
    Texture2D shootingTexture = { 0 };
    Texture2D idleGunTexture = { 0 };
    Texture2D shootingGunTexture = { 0 };

    BodyAnimation bodyAnimation = BodyAnimation::Idle;
    float bodyFrameTimer = 0.0f;
    int bodyFrameIndex = 0;
    bool gunShooting = false;
    float gunFrameTimer = 0.0f;
    float gunShotTimer = 0.0f;
    int gunFrameIndex = 0;
    bool nextWanderGoalUsesLineOfSight = false;

    /// Returns the current desired body animation.
    BodyAnimation GetDesiredBodyAnimation() const;
    /// Updates animations.
    void UpdateAnimations(float deltaTime);
    /// Calculates gun pose.
    GunPose CalculateGunPose(
        Vector2 entityPosition,
        Vector2 targetPosition
    ) const;
    /// Attempts to fire at active player.
    bool TryFireAtActivePlayer();

public:
    /// Creates a DemonTHA instance from the supplied configuration.
    DemonTHA(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess,
        ILevelLineOfSightQuery& lineOfSightQuery
    );
    /// Releases resources owned by this DemonTHA instance.
    ~DemonTHA() override;

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;

    /// Returns the current wander idle state.
    DemonTHAWanderIdleState* GetWanderIdleState() const {
        return wanderIdleState.get();
    }
    /// Returns the current wander move state.
    DemonTHAWanderMoveState* GetWanderMoveState() const {
        return wanderMoveState.get();
    }
    /// Returns the current aggro state.
    DemonTHAAggroState* GetAggroState() const {
        return aggroState.get();
    }
    /// Reports whether the aggroing condition is satisfied.
    bool IsAggroing() const;

    /// Returns the current active target.
    Paladin* GetActiveTarget() const;
    /// Reports whether this component has clear shot from.
    bool HasClearShotFrom(
        Vector2 entityPosition,
        const Paladin& target
    ) const;
    /// Reports whether this component should perform immediately aggro.
    bool ShouldImmediatelyAggro() const;
    /// Reports whether this component should perform roll distant aggro on idle entry.
    bool ShouldRollDistantAggroOnIdleEntry() const;
    /// Consumes and returns next wander goal uses line of sight.
    bool ConsumeNextWanderGoalUsesLineOfSight();
    /// Returns the current current room candidate radius.
    float GetCurrentRoomCandidateRadius() const;
    /// Updates the stored gun shooting.
    void SetGunShooting(bool shooting);
};
