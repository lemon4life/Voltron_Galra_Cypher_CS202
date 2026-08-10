#pragma once
#include "Entities/Player/Paladin.h"

class Hunk : public Paladin {
public:
    Hunk(Vector2 pos, CharacterSprites sprites);
    
    void UseSkill() override;
    void UseUltimate() override;
    void Update(float deltaTime) override;
    void Draw() override;
    void ApplyKnockback(Vector2 dir, float force) override;
    void ExecuteUltimateAction() override;
    
private:
    
    // Earthshatter mechanics
    float knockbackRadius = 100.0f;
    float knockbackForce = 2000.0f;
    bool isEarthshatterFlash = false;
    float earthshatterFlashTimer = 0.0f;
};
