#include "Entities/Player/Keith.h"
#include "Combat/MeleeAttackStrategy.h"

Keith::Keith(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, 100, 150.0f) // 100 HP, 150 Max Ex Energy
{
    speed = 220.0f;
    currentWeapon = new MeleeAttackStrategy(sprites.weapon, sprites.attack1, sprites.attack2);
    texture = GetIdleTexture();
}
