#pragma once

#include "Entities/Enemy.h"

class Boss : public Enemy {
private:
    float bossSkillCooldown;
    int burstCount;
    float burstTimer;

    std::unique_ptr<BossRangedAttackState> rangeState;
public:
    Boss(Vector2 pos, TeamManager* targetTeam);
    ~Boss() override;

    void Update(float deltaTime) override;
    void Draw() override;

    float GetBossSkillCooldown() const { return bossSkillCooldown; }
    void SetBossSkillCooldown(float cd) { bossSkillCooldown = cd; }
    int GetBurstCount() const { return burstCount; }
    void SetBurstCount(int count) { burstCount = count; }
    float GetBurstTimer() const { return burstTimer; }
    void SetBurstTimer(float timer) { burstTimer = timer; }
    BossRangedAttackState* GetBossRangedAttackState() { return rangeState.get(); }
};
