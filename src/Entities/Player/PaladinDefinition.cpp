#include "Entities/Player/PaladinDefinition.h"

#include <cstddef>

namespace {
const std::array<PaladinDefinition, 4> PALADINS = {{
    {
        PaladinId::Lance,
        "Lance",
        "A mobile sharpshooter with rapid, reliable ranged attacks.",
        "Lance_Idle",
        250,
        190.0f,
        100.0f,
        0.2f,
        {
            "Blue Bayard",
            "A fast energy rifle built for accurate ranged pressure.",
            "Lance_Weapon",
            34,
            34,
            15.0f,
            true
        }
    },
    {
        PaladinId::Keith,
        "Keith",
        "The fastest Paladin, specializing in aggressive close combat.",
        "Keith_Idle",
        200,
        220.0f,
        150.0f,
        0.2f,
        {
            "Red Bayard",
            "A melee blade with alternating light and heavy combo strikes.",
            "Keith_Weapon",
            50,
            80,
            0.0f,
            false
        }
    },
    {
        PaladinId::Hunk,
        "Hunk",
        "A durable heavy fighter with powerful piercing laser attacks.",
        "Hunk_Idle",
        300,
        150.0f,
        150.0f,
        0.5f,
        {
            "Yellow Bayard",
            "A heavy cannon whose beam pierces targets with strong recoil.",
            "Hunk_Weapon",
            50,
            50,
            30.0f,
            true
        }
    },
    {
        PaladinId::Pidge,
        "Pidge",
        "Tech Specialist. Boomerang attack, Poison Zone skill, Rover ultimate.",
        "Paladin_Pidge_Idle",
        90,    // maxHealth (slightly lower HP 0.9x)
        138.f, // speed (high speed 1.15x)
        100.f, // maxExEnergy
        0.4f,  // attackCooldown
        {
            "Green Bayard",
            "Returning boomerang",
            "Paladin_Pidge_Weapon",
            8,     // minimumDamage (slightly lower base damage)
            12,    // maximumDamage
            0.0f,  // recoil
            false  // recoilApplicable
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
