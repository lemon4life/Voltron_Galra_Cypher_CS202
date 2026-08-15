#pragma once

#include "Entities/Enemy.h"

class Boss : public Enemy {
private:
    std::unique_ptr<BossSpellingState> spellingState;
    std::unique_ptr<BossPunchState> punchState;
    Texture2D spellTexture = { 0 };
    Texture2D punchReadyTexture = { 0 };
    Texture2D punchBodyTexture = { 0 };
    Texture2D punchHandTexture = { 0 };
    Texture2D firePunchTexture = { 0 };

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
    BossPunchState* GetPunchState() { return punchState.get(); }
    bool IsSpelling() const { return currentState == spellingState.get(); }
    bool IsPunching() const { return currentState == punchState.get(); }
    bool TrySummonRandomEnemy();
    void FirePunchProjectile(
        float bulletSpeed,
        float changeAngleDegreesPerSecond
    );
    void ResetAnimationCycle();
};
