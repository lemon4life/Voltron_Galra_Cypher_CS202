#include "Entities/Player/PaladinDefinition.h"

#include <cstddef>

namespace {
const std::array<PaladinDefinition, 4> PALADINS = {{
    {
        PaladinId::Lance,
        "Lance",
        "A mobile sharpshooter with rapid, reliable ranged attacks.",
        "Lance_Idle",
        2.5f,
        0.95f,
        100.0f,
        0.4f,
        {
            "Blue Bayard",
            "A fast energy rifle built for accurate ranged pressure.",
            "Lance_Weapon",
            1.8f,
            1.8f,
            15.0f,
            true
        }
    },
    {
        PaladinId::Keith,
        "Keith",
        "The fastest Paladin, specializing in aggressive close combat.",
        "Keith_Idle",
        2.0f,
        1.1f,
        150.0f,
        0.4f,
        {
            "Red Bayard",
            "A melee blade with alternating light and heavy combo strikes.",
            "Keith_Weapon",
            2.5f,
            4.0f,
            0.0f,
            false
        }
    },
    {
        PaladinId::Hunk,
        "Hunk",
        "A durable heavy fighter with powerful piercing laser attacks.",
        "Hunk_Idle",
        3.0f,
        0.75f,
        150.0f,
        1.0f,
        {
            "Yellow Bayard",
            "A heavy cannon whose beam pierces targets with strong recoil.",
            "Hunk_Weapon",
            2.5f,
            2.5f,
            30.0f,
            true
        }
    },
    {
        PaladinId::Pidge,
        "Pidge",
        "Tech Specialist. Boomerang attack, Poison Zone skill, Rover ultimate.",
        "Paladin_Pidge_Idle",
        1.5f,    // hpScalar
        1.15f,   // speedScalar
        100.f,   // maxExEnergy
        0.8f,    // attackCooldownScalar
        {
            "Green Bayard",
            "Returning boomerang",
            "Paladin_Pidge_Weapon",
            1.6f,    // minDamageScalar
            1.8f,    // maxDamageScalar
            0.0f,    // recoil
            false    // recoilApplicable
        }
    }
}};
}

const PaladinDefinition& PaladinCatalog::Get(PaladinId id) {
    return PALADINS[static_cast<std::size_t>(id)];
}

const std::array<PaladinDefinition, 4>& PaladinCatalog::GetAll() {
    return PALADINS;
}
