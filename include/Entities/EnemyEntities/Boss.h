#pragma once

#include "Entities/Enemy.h"

class Boss : public Enemy {
private:
    std::unique_ptr<BossSpellingState> spellingState;
    Texture2D spellTexture = { 0 };

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
    BossSpellingState* GetSpellingState() { return spellingState.get(); }
    bool IsSpelling() const { return currentState == spellingState.get(); }
    bool TrySummonRandomEnemy();
    void ResetAnimationCycle();
};
