#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "raylib.h"
#include "Core/Manager/GameManager.h"

#include <algorithm>

TeamManager::TeamManager()
    : activeIndex(0),
      sharedUltimateDecibels(0.0f)
{
}

TeamManager::~TeamManager() {
    for (auto* paladin : team) {
        delete paladin;
    }
    team.clear();
}

void TeamManager::AddMember(Paladin* paladin) {
    if (team.size() < 3) {
        team.push_back(paladin);
        paladin->SetTeamManager(this);
    }
}

Paladin* TeamManager::GetActivePaladin() const {
    if (team.empty()) return nullptr;
    return team[activeIndex];
}

int TeamManager::FindMemberIndex(PaladinId id) const {
    for (std::size_t index = 0; index < team.size(); ++index) {
        if (team[index] && team[index]->GetPaladinId() == id) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

bool TeamManager::MovePaladinToSlot(
    PaladinId id,
    std::size_t targetIndex
) {
    if (targetIndex >= team.size()) {
        return false;
    }

    int sourceIndex = FindMemberIndex(id);
    if (sourceIndex < 0) {
        return false;
    }
    if (sourceIndex == static_cast<int>(targetIndex)) {
        return true;
    }

    Paladin* activePaladin = GetActivePaladin();
    std::swap(team[static_cast<std::size_t>(sourceIndex)], team[targetIndex]);

    auto activeIt = std::find(team.begin(), team.end(), activePaladin);
    if (activeIt != team.end()) {
        activeIndex = static_cast<int>(activeIt - team.begin());
    }

    NotifyObservers();
    return true;
}

void TeamManager::AddDepthRenderItems(std::vector<DepthRenderItem>& items) {
    if (activeIndex >= 0 && activeIndex < team.size()) {
        Paladin* activePaladin = team[activeIndex];
        items.push_back({
            activePaladin->GetBoundingBox().y + activePaladin->GetBoundingBox().height,
            [activePaladin]() { activePaladin->Draw(); }
        });
    }
}

void TeamManager::SwapCharacter() {
    if (team.size() <= 1) return;
    
    Paladin* oldActive = GetActivePaladin();
    if (!oldActive) return;

    int attempts = 0;
    int nextIndex = activeIndex;
    do {
        nextIndex = (nextIndex + 1) % team.size();
        attempts++;
    } while (team[nextIndex]->GetHealth() <= 0 && attempts < team.size());

    if (attempts >= team.size()) return; // Everyone is dead

    activeIndex = nextIndex;
    Paladin* newActive = GetActivePaladin();
    
    // Transfer position and aim target
    newActive->SetPosition(oldActive->GetPosition());
    newActive->SetAimTarget(oldActive->GetAimTarget());
    newActive->TriggerSwapParryWindow();
    
    NotifyObservers();
}

void TeamManager::SwapDueToDeath() {
    if (team.size() <= 1) return;
    
    Paladin* deadActive = GetActivePaladin();
    if (!deadActive) return;
    
    // Move to the back of the team queue
    team.erase(team.begin() + activeIndex);
    team.push_back(deadActive);
    
    // activeIndex is now effectively pointing to the "next" character in the old array
    // We need to find the first alive character starting from 0 (since we shifted everything left)
    int attempts = 0;
    int nextIndex = 0;
    bool foundAlive = false;
    
    while (attempts < team.size()) {
        if (team[nextIndex]->GetHealth() > 0) {
            foundAlive = true;
            break;
        }
        nextIndex = (nextIndex + 1) % team.size();
        attempts++;
    }
    
    if (!foundAlive) return;
    
    activeIndex = nextIndex;
    Paladin* newActive = GetActivePaladin();
    
    // Transfer position and aim target
    newActive->SetPosition(deadActive->GetPosition());
    newActive->SetAimTarget(deadActive->GetAimTarget());
    
    NotifyObservers();
}

void TeamManager::Update(float deltaTime) {
    if (team.empty()) return;

    // Check if current active paladin is dead is now handled by PlayerDownState deferred logic.

    if (IsKeyPressed(KEY_TAB)) {
        SwapCharacter();
    }



    // Only update the active paladin
    GetActivePaladin()->Update(deltaTime);
}

void TeamManager::Draw() {
    if (team.empty()) return;
    GetActivePaladin()->Draw();
}



void TeamManager::NotifyObservers() {
    Paladin* active = GetActivePaladin();
    if (!active) return;
    
    for (auto* observer : observers) {
        // Observers will need to be updated to handle the new stats format.
        // We will pass the active paladin's HP and the shared armor.
        // Note: isPlayingAsLance boolean flag is now obsolete.
        observer->OnPlayerStatsChanged(active->GetHealth(), active->GetMaxHealth(), 0, 0, activeIndex == 0);
    }
}
bool TeamManager::IsTeamDead() const {
    for (auto* paladin : team) {
        if (paladin->GetHealth() > 0) return false;
    }
    return true;
}
