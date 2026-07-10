#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "raylib.h"

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
    activeIndex = (activeIndex + 1) % team.size();
    Paladin* newActive = GetActivePaladin();
    
    // Transfer position and aim target
    newActive->SetPosition(oldActive->GetPosition());
    newActive->SetAimTarget(oldActive->GetAimTarget());
    
    // Optional: play swap effect, brief invincibility, etc.
    
    NotifyObservers();
}

void TeamManager::Update(float deltaTime) {
    if (team.empty()) return;

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
