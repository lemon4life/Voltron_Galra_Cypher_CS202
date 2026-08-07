#include "Entities/Player/Lance.h"
#include "Combat/RangedAttackStrategy.h"
#include "Entities/Player/PaladinDefinition.h"

Lance::Lance(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, PaladinCatalog::Get(PaladinId::Lance))
{
    const WeaponDefinition& weapon =
        PaladinCatalog::Get(PaladinId::Lance).weapon;
    currentWeapon = new RangedAttackStrategy(
        sprites.weapon,
        sprites.muzzleFlash,
        sprites.bullet,
        weapon.maximumDamage,
        weapon.recoil
    );
    texture = GetIdleTexture();
}

void Lance::UseSkill() {
    // TODO: Implement Lance's unique skill
}

void Lance::UseUltimate() {
    // TODO: Implement Lance's unique ultimate
}
