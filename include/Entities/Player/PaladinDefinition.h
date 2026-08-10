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
    int minimumDamage;
    int maximumDamage;
    float recoil;
    bool recoilApplicable;
};

struct PaladinDefinition {
    PaladinId id;
    std::string name;
    std::string description;
    std::string idleTextureKey;
    int maxHealth;
    float speed;
    float maxExEnergy;
    float attackCooldown;
    WeaponDefinition weapon;
};

class PaladinCatalog {
public:
    static const PaladinDefinition& Get(PaladinId id);
    static const std::array<PaladinDefinition, 4>& GetAll();
};
