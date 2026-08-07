#include "Entities/Player/Hunk.h"
#include "Combat/LaserAttackStrategy.h"
#include "Entities/Player/PaladinDefinition.h"

Hunk::Hunk(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, PaladinCatalog::Get(PaladinId::Hunk))
{
    const WeaponDefinition& weapon =
        PaladinCatalog::Get(PaladinId::Hunk).weapon;
    currentWeapon = new LaserAttackStrategy(
        sprites.weapon,
        sprites.muzzleFlash,
        sprites.bullet,
        sprites.impact,
        weapon.maximumDamage,
        weapon.recoil
    );
    texture = GetIdleTexture();
}

void Hunk::UseSkill() {
    // TODO: Implement Hunk's unique skill
}

void Hunk::UseUltimate() {
    // TODO: Implement Hunk's unique ultimate
}
