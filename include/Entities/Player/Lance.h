#pragma once
#include "Entities/Player/Paladin.h"

class Lance : public Paladin {
public:
    /// Creates a Lance instance from the supplied configuration.
    Lance(Vector2 pos, CharacterSprites sprites);
    
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Updates inactive.
    void UpdateInactive(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    /// Starts this attack behavior when its current conditions allow it.
    void Attack() override;
    /// Activates skill.
    void UseSkill() override;
    /// Activates ultimate.
    void UseUltimate() override;
    /// Executes the gameplay effect after the Ultimate introduction finishes.
    void ExecuteUltimateAction() override;
    /// Reports whether the weapon visible condition is satisfied.
    bool IsWeaponVisible() const override;
    
private:
    struct PendingFreezeTarget {
        ObjectId enemyId = INVALID_OBJECT_ID;
        float delay;
    };
    std::vector<PendingFreezeTarget> pendingFreezeTargets;
    /// Updates pending freeze targets.
    void UpdatePendingFreezeTargets(float deltaTime);
    
    bool isUltimateFlash;
    float ultimateFlashTimer;
};
