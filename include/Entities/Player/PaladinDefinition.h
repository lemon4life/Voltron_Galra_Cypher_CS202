#pragma once

#include <array>
#include <string>

enum class PaladinId {
    Lance,
    Keith,
    Hunk,
    Pidge
};

struct AbilityInfo {
    std::string name;
    std::string description;
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

// Design Pattern - Data-Driven Catalog:
// PaladinDefinition stores identity, stats, descriptions, ability info, and asset keys as data.
// PaladinCatalog is the central read-only lookup used by gameplay and UI, avoiding
// repeated character metadata and selection switches across concrete Paladins.
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
    AbilityInfo skill;
    AbilityInfo ultimate;
};

class PaladinCatalog {
public:
    /// Returns the value represented by this accessor object.
    static const PaladinDefinition& Get(PaladinId id);
    /// Returns the current all.
    static const std::array<PaladinDefinition, 4>& GetAll();
};
