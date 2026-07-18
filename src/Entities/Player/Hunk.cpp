#include "Entities/Player/Hunk.h"
#include "Combat/LaserAttackStrategy.h"

Hunk::Hunk(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, 300, 150.0f) // Hunk is tankier: 300 HP, 150 Max Ex
{
    speed = 100.0f; // Slower movement
    currentWeapon = new LaserAttackStrategy(sprites.weapon, sprites.muzzleFlash, sprites.bullet, sprites.impact);
    texture = GetIdleTexture();
}
