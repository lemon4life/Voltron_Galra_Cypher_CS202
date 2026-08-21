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
        Vector2 muzzleWorld = { 0.0f, 0.0f };
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
    int gunFrameIndex = 0;
    bool nextWanderGoalUsesLineOfSight = false;

    BodyAnimation GetDesiredBodyAnimation() const;
    void UpdateAnimations(float deltaTime);
    GunPose CalculateGunPose(
        Vector2 entityPosition,
        Vector2 targetPosition
    ) const;
    bool TryFireAtActivePlayer();

public:
    DemonTHA(
        Vector2 position,
        TeamManager* targetTeam,
        IEntityRemovalAccess& removalAccess,
        IEnemyPathAccess& pathAccess,
        ILevelLineOfSightQuery& lineOfSightQuery
    );
    ~DemonTHA() override;

    void Update(float deltaTime) override;
    void Draw() override;

    DemonTHAWanderIdleState* GetWanderIdleState() const {
        return wanderIdleState.get();
    }
    DemonTHAWanderMoveState* GetWanderMoveState() const {
        return wanderMoveState.get();
    }
    DemonTHAAggroState* GetAggroState() const {
        return aggroState.get();
    }
    bool IsAggroing() const;

    Paladin* GetActiveTarget() const;
    bool HasClearShotFrom(
        Vector2 entityPosition,
        const Paladin& target
    ) const;
    bool ShouldImmediatelyAggro() const;
    bool ShouldRollDistantAggroOnIdleEntry() const;
    bool ConsumeNextWanderGoalUsesLineOfSight();
    float GetCurrentRoomCandidateRadius() const;
    void SetGunShooting(bool shooting);
};
