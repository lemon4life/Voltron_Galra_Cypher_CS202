#pragma once

struct PlayerStatsSnapshot {
    int slotIndex = 0;
    int health = 0;
    int maxHealth = 0;
    float displayedHp = 0.0f;
    float ghostHp = 0.0f;
    float exEnergy = 0.0f;
    float displayedEx = 0.0f;
    float maxEx = 0.0f;
    float skillCost = 0.0f;
    bool isDowned = false;
};

struct TeamStatsSnapshot {
    int activeIndex = 0;
    int sharedArmor = 0;
    int maxSharedArmor = 0;
    float currentQuintessence = 0.0f;
    float displayedQuintessence = 0.0f;
    float maxQuintessence = 300.0f;
};

class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void OnPlayerStatsChanged(const PlayerStatsSnapshot& stats, int slotIndex) = 0;
    virtual void OnTeamStatsChanged(const TeamStatsSnapshot& stats) = 0;
};

