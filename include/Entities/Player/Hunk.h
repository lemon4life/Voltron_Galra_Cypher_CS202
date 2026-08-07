#pragma once
#include "Entities/Player/Paladin.h"

class Hunk : public Paladin {
public:
    Hunk(Vector2 pos, CharacterSprites sprites);
    
    void UseSkill() override;
    void UseUltimate() override;
};
