#include "Entities/Player/Keith.h"
#include "Combat/MeleeAttackStrategy.h"
#include "Core/Manager/AssetManager.h"

Keith::Keith(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, 200, 150.0f) // 100 HP, 150 Max Ex Energy
{
    speed = 220.0f;
    currentWeapon = new MeleeAttackStrategy(sprites.weapon, AssetManager::GetInstance().GetTexture("Sword_Slash_Small"), AssetManager::GetInstance().GetTexture("Sword_Slash_Small"));
    texture = GetIdleTexture();
}
