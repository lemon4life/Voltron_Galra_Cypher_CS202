#pragma once
#include "Entities/Player/Paladin.h"

class Keith : public Paladin {
public:
    Keith(Vector2 pos, CharacterSprites sprites);
    
    void UseSkill() override;
    void UseUltimate() override;
    void ExecuteUltimateAction() override;
    
    void Update(float deltaTime) override;
    void Draw() override;
    
    void UpdateInactive(float deltaTime) override;
    void DrawInactive() override;
    bool IsDoingUltimate() const override { return isUltimateAiming || ultimateFlashTimer > 0.0f; }
    
private:
    float ultimateFlashTimer = 0.0f;
    bool isUltimateAiming = false;
    
    float skillCooldownTimer = 0.0f;
    const float SKILL_COOLDOWN = 10.0f;
    
public:
    bool debugSpamMode = true; // Debug toggle for instant spam
};
