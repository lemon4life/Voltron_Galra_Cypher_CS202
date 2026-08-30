#pragma once
#include "Entities/Player/Paladin.h"

class Keith : public Paladin {
public:
    /// Creates a Keith instance from the supplied configuration.
    Keith(Vector2 pos, CharacterSprites sprites);
    
    /// Activates skill.
    void UseSkill() override;
    /// Activates ultimate.
    void UseUltimate() override;
    /// Executes the gameplay effect after the Ultimate introduction finishes.
    void ExecuteUltimateAction() override;
    
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    
    /// Updates inactive.
    void UpdateInactive(float deltaTime) override;
    /// Renders inactive.
    void DrawInactive() override;
private:
    bool isUltimateAiming = false;
    float skillCooldownTimer = 0.0f;
    const float SKILL_COOLDOWN = 10.0f;
    
public:
    /// Reports whether the doing ultimate condition is satisfied.
    bool IsDoingUltimate() const override { return isUltimateAiming; }
};
