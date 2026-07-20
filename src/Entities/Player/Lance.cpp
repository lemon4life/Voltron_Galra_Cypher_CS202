#include "Entities/Player/Lance.h"
#include "Combat/RangedAttackStrategy.h"

Lance::Lance(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, 150, 100.0f) // 150 HP, 100 Max Ex Energy
{
    speed = 190.0f;
    attackCooldown = 0.2f;
    currentWeapon = new RangedAttackStrategy(sprites.weapon, sprites.muzzleFlash, sprites.bullet);
    texture = GetIdleTexture();
}
