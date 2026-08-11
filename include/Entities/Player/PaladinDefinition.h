#pragma once

#include <array>
#include <string>

enum class PaladinId {
    Lance,
    Keith,
    Hunk,
    Pidge
};

struct WeaponDefinition {
    std::string name;
    std::string description;
    std::string textureKey;
    float minDamageScalar;
    float maxDamageScalar;
    float recoil;
    bool recoilApplicable;
};

struct PaladinDefinition {
    PaladinId id;
    std::string name;
    std::string description;
    std::string idleTextureKey;
    float hpScalar;
    float speedScalar;
    float maxExEnergy;
    float attackCooldownScalar;
    WeaponDefinition weapon;
};

class PaladinCatalog {
public:
    static const PaladinDefinition& Get(PaladinId id);
    static const std::array<PaladinDefinition, 4>& GetAll();
};
