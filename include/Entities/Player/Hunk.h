#pragma once
#include "Entities/Player/Paladin.h"

class Hunk : public Paladin {
public:
    /// Creates a Hunk instance from the supplied configuration.
    Hunk(Vector2 pos, CharacterSprites sprites);
    
    /// Activates skill.
    void UseSkill() override;
    /// Activates ultimate.
    void UseUltimate() override;
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
    /// Applies knockback.
    void ApplyKnockback(Vector2 dir, float force) override;
    /// Executes the gameplay effect after the Ultimate introduction finishes.
    void ExecuteUltimateAction() override;
    
private:
    
    // Earthshatter mechanics
    float knockbackRadius = 100.0f;
    float knockbackForce = 2000.0f;
    bool isEarthshatterFlash = false;
    float earthshatterFlashTimer = 0.0f;
};
