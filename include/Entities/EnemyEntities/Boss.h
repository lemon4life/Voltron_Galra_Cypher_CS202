#pragma once

#include "Entities/Enemy.h"

class Boss : public Enemy {
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

    BossIdlingState* GetIdlingState() {
        return static_cast<BossIdlingState*>(idleState.get());
    }
};
