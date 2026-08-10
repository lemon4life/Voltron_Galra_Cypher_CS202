#pragma once
#include "Entities/Player/Paladin.h"

class Lance : public Paladin {
public:
    Lance(Vector2 pos, CharacterSprites sprites);
    
    void Update(float deltaTime) override;
    void Draw() override;
    void Attack() override;
    void UseSkill() override;
    void UseUltimate() override;
    void ExecuteUltimateAction() override;
    
private:
    
    bool isUltimateFlash;
    float ultimateFlashTimer;
};
