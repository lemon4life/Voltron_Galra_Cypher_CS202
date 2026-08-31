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
            "Fires rapid, high-velocity laser rifle bolts with steady recoil kickback and pinpoint forward accuracy.",
            "Lance_Weapon",
            1.8f,
            1.8f,
            15.0f,
            true
        },
        {
            "Dual Wield",
            "Equips dual Blue Bayards for 5s, doubling fire rate to unleash synchronized double volleys."
        },
        {
            "Glacier Pierce",
            "Detonates cryogenic explosions under all enemies on the battlefield, freezing every hostile solid for 5s."
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
            "Slices enemies in a sweeping 2-hit melee combo with bonus critical strike damage on the second swing.",
            "Keith_Weapon",
            2.5f,
            4.0f,
            0.0f,
            false
        },
        {
            "Fire Circle",
            "Ignites a rotating fiery aura for 5s that continuously burns nearby enemies and generates bonus EX energy."
        },
        {
            "Excalibur",
            "Charges and unleashes a massive flaming energy wave across 500px, leaving a lingering fire trail that incinerates foes."
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
            "Fires a continuous piercing laser beam that tears through multiple lined-up targets with heavy recoil.",
            "Hunk_Weapon",
            2.5f,
            2.5f,
            30.0f,
            true
        },
        {
            "Earthshatter",
            "Slams the ground to emit a seismic shockwave, knocking back surrounding enemies and stunning them for 2s."
        },
        {
            "Aegis Shield",
            "Deploys an impenetrable rotating Aegis barrier around the team, granting complete invulnerability for 5s."
        }
    },
    {
        PaladinId::Pidge,
        "Pidge",
        "An agile tech specialist with versatile tethered grappling attacks.",
        "Paladin_Pidge_Idle",
        1.5f,    // hpScalar
        1.15f,   // speedScalar
        100.f,   // maxExEnergy
        0.55f,   // attackCooldownScalar
        {
            "Green Bayard",
            "Launches a grappling katar forward on an electric cable that pierces targets and snaps back to hand with zero recoil.",
            "Paladin_Pidge_Weapon",
            1.6f,    // minDamageScalar
            1.8f,    // maxDamageScalar
            0.0f,    // recoil
            false    // recoilApplicable
        },
        {
            "Venom Zone",
            "Deploys a caustic chemical field for 7s that inflicts lingering poison damage (DoT) and severely slows enemy movement."
        },
        {
            "Rover Override",
            "Deploys Rover, an autonomous flying combat drone companion that follows the team and provides heavy fire support."
        }
    }
}};
}

/// Returns the value represented by this accessor object.
const PaladinDefinition& PaladinCatalog::Get(PaladinId id) {
    return PALADINS[static_cast<std::size_t>(id)];
}

/// Returns the current all.
const std::array<PaladinDefinition, 4>& PaladinCatalog::GetAll() {
    return PALADINS;
}
