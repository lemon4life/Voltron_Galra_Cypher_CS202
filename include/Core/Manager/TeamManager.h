#pragma once
#include <cstddef>
#include <vector>
#include "raylib.h"
#include "Core/ISubject.h"
#include "Entities/Player/PaladinDefinition.h"
#include "Core/DepthRenderItem.h"
#include "Combat/IBuff.h"
#include <memory>

class Paladin;

class TeamManager : public ISubject {
private:
    std::vector<Paladin*> roster;
    std::vector<Paladin*> team;
    int activeIndex;

    int sharedArmor;
    int maxSharedArmor;
    float sharedUltimateDecibels;

    float timeSinceLastDamage;
    float armorRegenTimer;

    std::vector<std::unique_ptr<IBuff>> sharedBuffs;

public:
    TeamManager();
    ~TeamManager();

    void AddMember(Paladin* paladin);
    void Update(float deltaTime);
    void Draw();
    void DrawBuffs(); // Draw shared buffs
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items);
    
    void AddSharedBuff(std::unique_ptr<IBuff> buff) {
        if (buff && GetActivePaladin()) {
            buff->OnApply(GetActivePaladin());
            sharedBuffs.push_back(std::move(buff));
        }
    }

    void SwapCharacter();
    void SwapCharacterToIndex(int targetIndex);
    void SwapDueToDeath();
    void ResetForNewGame(Vector2 spawnPosition);
    int FindMemberIndex(PaladinId id) const;
    bool AssignPaladinToSlot(PaladinId id, std::size_t targetIndex);
    Paladin* GetActivePaladin() const;

    int TakeArmorDamage(int amount);
    bool IsTeamDead() const; // Returns remaining damage that penetrates armor
    void RecordDamageEvent();
    
    int GetSharedArmor() const { return sharedArmor; }
    int GetMaxSharedArmor() const { return maxSharedArmor; }
    
    // Observers might need to know when team state changes
    void NotifyObservers() override;
    
    const std::vector<Paladin*>& GetTeam() const { return team; }
    int GetActiveIndex() const { return activeIndex; }
};
