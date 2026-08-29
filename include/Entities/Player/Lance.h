#pragma once
#include "Entities/Player/Paladin.h"

class Lance : public Paladin {
public:
    Lance(Vector2 pos, CharacterSprites sprites);
    
    void Update(float deltaTime) override;
    void UpdateInactive(float deltaTime) override;
    void Draw() override;
    void Attack() override;
    void UseSkill() override;
    void UseUltimate() override;
    void ExecuteUltimateAction() override;
    bool IsWeaponVisible() const override;
    
private:
    struct PendingFreezeTarget {
        ObjectId enemyId = INVALID_OBJECT_ID;
        float delay;
    };
    std::vector<PendingFreezeTarget> pendingFreezeTargets;
    void UpdatePendingFreezeTargets(float deltaTime);
    
    bool isUltimateFlash;
    float ultimateFlashTimer;
};
