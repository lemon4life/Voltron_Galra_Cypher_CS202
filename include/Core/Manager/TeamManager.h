#pragma once
#include <cstddef>
#include <vector>
#include "raylib.h"
#include "Core/ISubject.h"
#include "Entities/Player/PaladinDefinition.h"
#include "Core/DepthRenderItem.h"
#include "Combat/IBuff.h"
#include "Core/AimStrategy/IAimStrategy.h"
#include "Core/AimStrategy/AutoAimStrategy.h"
#include "Core/AimStrategy/MouseAimStrategy.h"
#include "Core/MissionSaveData.h"
#include <memory>

class Paladin;

// Design Patterns - Observer subject and Strategy selector:
// TeamManager publishes player/team snapshots to IObserver implementations and
// owns both IAimStrategy implementations, selecting one for the Paladin contexts.
// Its unique_ptr roster and buffs also make lifetime ownership explicit through RAII.
class TeamManager : public ISubject {
private:
    std::vector<std::unique_ptr<Paladin>> roster;
    std::vector<Paladin*> team;
    int activeIndex;

    int sharedArmor;
    int maxSharedArmor;

    // Quintessence — shared team ultimate fuel (separate from per-character EX)
    float currentQuintessence = 0.0f;
    float displayedQuintessence = 0.0f;
    float maxQuintessence = 300.0f;
    bool debugFastFuel = false;

    // Coins Currency
    int coins = 0;

    std::vector<std::unique_ptr<IBuff>> sharedBuffs;

    bool isSpawning = false;
    float spawnAnimTimer = 0.0f;

    // Aim strategies owned by the team — shared across all Paladins via raw ptr
    std::unique_ptr<IAimStrategy> autoStrategy;
    std::unique_ptr<IAimStrategy> mouseStrategy;

public:
    /// Creates a TeamManager instance from the supplied configuration.
    TeamManager();
    /// Releases resources owned by this TeamManager instance.
    ~TeamManager();

    /// Adds member.
    void AddMember(std::unique_ptr<Paladin> paladin);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime);
    /// Renders this component using its current state and visual resources.
    void Draw();
    /// Renders buffs.
    void DrawBuffs(); // Draw shared buffs
    /// Adds depth render items.
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items);
    /// Refreshes aim strategies.
    void RefreshAimStrategies(); // Apply the correct strategy to every team member
    /// Starts spawn animation.
    void StartSpawnAnimation();
    
    /// Adds shared buff.
    void AddSharedBuff(std::unique_ptr<IBuff> buff) {
        if (buff && GetActivePaladin()) {
            buff->OnApply(GetActivePaladin());
            sharedBuffs.push_back(std::move(buff));
        }
    }

    /// Swaps character.
    void SwapCharacter();
    /// Swaps character to index.
    void SwapCharacterToIndex(int targetIndex);
    /// Swaps due to death.
    void SwapDueToDeath();
    /// Resets for new game.
    void ResetForNewGame(Vector2 spawnPosition);
    /// Searches for member index.
    int FindMemberIndex(PaladinId id) const;
    /// Assigns paladin to slot.
    bool AssignPaladinToSlot(PaladinId id, std::size_t targetIndex);
    /// Returns the current active paladin.
    Paladin* GetActivePaladin() const;

    /// Reports whether the team dead condition is satisfied.
    bool IsTeamDead() const; // Returns remaining damage that penetrates armor
    
    /// Returns the current shared armor.
    int GetSharedArmor() const { return sharedArmor; }
    /// Returns the current max shared armor.
    int GetMaxSharedArmor() const { return maxSharedArmor; }

    // Quintessence (shared ultimate fuel)
    static constexpr float ULTIMATE_COST = 100.0f;
    /// Adds quintessence.
    void AddQuintessence(float amount);
    /// Consumes and returns quintessence.
    bool ConsumeQuintessence(float amount);
    /// Returns the current quintessence.
    float GetQuintessence() const { return currentQuintessence; }
    /// Returns the current displayed quintessence.
    float GetDisplayedQuintessence() const { return displayedQuintessence; }
    /// Returns the current max quintessence.
    float GetMaxQuintessence() const { return maxQuintessence; }

    // Coins Currency
    /// Returns the current coins.
    int GetCoins() const { return coins; }
    /// Adds coins.
    void AddCoins(int amount) {
        if (amount > 0) coins += amount;
    }
    /// Consumes and returns coins.
    bool ConsumeCoins(int amount) {
        if (amount > 0 && coins >= amount) {
            coins -= amount;
            return true;
        }
        return false;
    }
    
    // Observers might need to know when team state changes
    /// Notifies observers.
    void NotifyObservers() override;
    
    /// Returns the current team.
    const std::vector<Paladin*>& GetTeam() const { return team; }
    /// Returns the current active index.
    int GetActiveIndex() const { return activeIndex; }
    /// Captures selected slots, shared resources, and every Paladin's progression.
    SavedTeamState CaptureCheckpointState() const;
    /// Restores a checkpoint into the already loaded character roster.
    bool RestoreCheckpointState(const SavedTeamState& saved);
};
