#pragma once

class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void OnPlayerStatsChanged(int hp, int maxHp, int armor, int maxArmor, bool isLance) = 0;
};
