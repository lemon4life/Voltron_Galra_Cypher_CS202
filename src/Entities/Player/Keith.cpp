#include "Entities/Player/Keith.h"
#include "Combat/MeleeAttackStrategy.h"
#include "Core/Manager/AssetManager.h"
#include "Entities/Player/PaladinDefinition.h"

Keith::Keith(Vector2 pos, CharacterSprites sprites)
    : Paladin(pos, sprites, PaladinCatalog::Get(PaladinId::Keith))
{
    const WeaponDefinition& weapon =
        PaladinCatalog::Get(PaladinId::Keith).weapon;
    currentWeapon = new MeleeAttackStrategy(
        sprites.weapon,
        AssetManager::GetInstance().GetTexture("Sword_Slash_Small"),
        AssetManager::GetInstance().GetTexture("Sword_Slash_Small"),
        weapon.minimumDamage,
        weapon.maximumDamage
    );
    texture = GetIdleTexture();
}
