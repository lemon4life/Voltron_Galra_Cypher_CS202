#pragma once
#include <vector>
#include "Core/ISubject.h"

class Paladin;

class TeamManager : public ISubject {
private:
    std::vector<Paladin*> team;
    int activeIndex;

    int sharedArmor;
    int maxSharedArmor;
    float sharedUltimateDecibels;

    float timeSinceLastDamage;
    float armorRegenTimer;

public:
    TeamManager();
    ~TeamManager();

    void AddMember(Paladin* paladin);
    void Update(float deltaTime);
    void Draw();

    void SwapCharacter();
    Paladin* GetActivePaladin() const;

    int TakeArmorDamage(int amount); // Returns remaining damage that penetrates armor
    void RecordDamageEvent();
    
    int GetSharedArmor() const { return sharedArmor; }
    int GetMaxSharedArmor() const { return maxSharedArmor; }
    
    // Observers might need to know when team state changes
    void NotifyObservers() override;
    
    const std::vector<Paladin*>& GetTeam() const { return team; }
    int GetActiveIndex() const { return activeIndex; }
};
