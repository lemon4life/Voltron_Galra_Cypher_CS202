#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "raylib.h"
#include "Core/Manager/GameManager.h"

TeamManager::TeamManager()
    : activeIndex(0),
      sharedArmor(50),
      maxSharedArmor(50),
      sharedUltimateDecibels(0.0f),
      timeSinceLastDamage(0.0f),
      armorRegenTimer(0.0f)
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
    
    NotifyObservers();
}

void TeamManager::Update(float deltaTime) {
    if (team.empty()) return;

    // Check if current active paladin is dead
    Paladin* active = GetActivePaladin();
    if (active && active->GetHealth() <= 0) {
        Paladin* deadPaladin = active;
        
        // Move to the back of the team queue
        team.erase(team.begin() + activeIndex);
        team.push_back(deadPaladin);
        
        // Find the next available living Paladin
        int attempts = 0;
        bool foundAlive = false;
        while (attempts < team.size()) {
            if (activeIndex >= team.size()) activeIndex = 0;
            if (team[activeIndex]->GetHealth() > 0) {
                foundAlive = true;
                break;
            }
            activeIndex++;
            attempts++;
        }

        if (!foundAlive) {
            GameManager::GetInstance().SetState(GameState::GAMEOVER);
            return;
        }

        Paladin* newActive = GetActivePaladin();
        newActive->SetPosition(deadPaladin->GetPosition());
        newActive->SetAimTarget(deadPaladin->GetAimTarget());
        
        NotifyObservers();
    }

    if (IsKeyPressed(KEY_TAB)) {
        SwapCharacter();
    }

    // Handle Armor Regeneration
    timeSinceLastDamage += deltaTime;
    if (timeSinceLastDamage >= 3.0f && sharedArmor < maxSharedArmor) {
        armorRegenTimer += deltaTime;
        // Regenerate 10 armor per second (1 point per 0.1s)
        if (armorRegenTimer >= 0.1f) {
            sharedArmor += 1;
            armorRegenTimer = 0.0f;
            if (sharedArmor > maxSharedArmor) sharedArmor = maxSharedArmor;
            NotifyObservers();
        }
    }

    // Only update the active paladin
    GetActivePaladin()->Update(deltaTime);
}

void TeamManager::Draw() {
    if (team.empty()) return;
    GetActivePaladin()->Draw();
}

void TeamManager::RecordDamageEvent() {
    timeSinceLastDamage = 0.0f;
}

int TeamManager::TakeArmorDamage(int amount) {
    RecordDamageEvent();
    
    if (sharedArmor > 0) {
        sharedArmor -= amount;
        if (sharedArmor < 0) {
            int remainingDamage = -sharedArmor;
            sharedArmor = 0;
            NotifyObservers();
            return remainingDamage;
        }
        NotifyObservers();
        return 0; // Completely absorbed by armor
    }
    
    return amount;
}

void TeamManager::NotifyObservers() {
    Paladin* active = GetActivePaladin();
    if (!active) return;
    
    for (auto* observer : observers) {
        // Observers will need to be updated to handle the new stats format.
        // We will pass the active paladin's HP and the shared armor.
        // Note: isPlayingAsLance boolean flag is now obsolete.
        observer->OnPlayerStatsChanged(active->GetHealth(), active->GetMaxHealth(), sharedArmor, maxSharedArmor, activeIndex == 0);
    }
}
