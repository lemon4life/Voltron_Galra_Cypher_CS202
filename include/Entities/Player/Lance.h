#pragma once
#include "Entities/Player/Paladin.h"

class Lance : public Paladin {
public:
    Lance(Vector2 pos, CharacterSprites sprites);
    
    void UseSkill() override;
    void UseUltimate() override;
};
