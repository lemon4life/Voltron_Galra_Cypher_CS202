#include "Entities/Player/PlaceholderPaladin.h"
#include "Combat/MeleeAttackStrategy.h"

PlaceholderPaladin::PlaceholderPaladin(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, 120, 120.0f) // 120 HP, 120 Max Ex Energy
{
    speed = 180.0f;
    currentWeapon = new MeleeAttackStrategy(sprites.weapon, sprites.weapon, sprites.weapon); // Reusing melee strategy for now
    texture = GetIdleTexture();
}
