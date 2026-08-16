#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "raylib.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/InputManager.h"
#include "Core/Constants.h"
#include <algorithm>

TeamManager::TeamManager()
    : activeIndex(0),
      sharedArmor(0),
      maxSharedArmor(0),
      sharedUltimateDecibels(0.0f),
      timeSinceLastDamage(0.0f),
      armorRegenTimer(0.0f),
      autoStrategy(std::make_unique<AutoAimStrategy>()),
      mouseStrategy(std::make_unique<MouseAimStrategy>())
{
}

TeamManager::~TeamManager() {
    for (auto* paladin : roster) {
        delete paladin;
    }
    roster.clear();
    team.clear();
}

void TeamManager::AddMember(Paladin* paladin) {
    roster.push_back(paladin);
    if (team.size() < 3) {
        team.push_back(paladin);
    }
    paladin->SetTeamManager(this);
}

void TeamManager::RefreshAimStrategies() {
    bool useAuto = Constants::isAutoAimEnabled ||
                   InputManager::GetMode() == InputMode::KEYBOARD_ONLY;
    IAimStrategy* chosen = useAuto ? autoStrategy.get() : mouseStrategy.get();
    for (auto* p : team) {
        p->SetCurrentAimStrategy(chosen);
    }
}

Paladin* TeamManager::GetActivePaladin() const {
    if (team.empty()) return nullptr;
    return team[activeIndex];
}

void TeamManager::ResetForNewGame(Vector2 spawnPosition) {
    std::sort(
        team.begin(),
        team.end(),
        [](const Paladin* left, const Paladin* right) {
            return static_cast<int>(left->GetPaladinId()) <
                static_cast<int>(right->GetPaladinId());
        }
    );
    activeIndex = 0;
    sharedArmor = maxSharedArmor;
    sharedUltimateDecibels = 0.0f;
    currentQuintessence = 0.0f;
    displayedQuintessence = 0.0f;
    timeSinceLastDamage = 0.0f;
    armorRegenTimer = 0.0f;

    for (Paladin* paladin : roster) {
        paladin->SetPosition(spawnPosition);
        paladin->ResetStats();
        paladin->SetAimTarget(spawnPosition);
    }
    NotifyObservers();
}

int TeamManager::FindMemberIndex(PaladinId id) const {
    for (std::size_t index = 0; index < team.size(); ++index) {
        if (team[index] && team[index]->GetPaladinId() == id) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

bool TeamManager::AssignPaladinToSlot(
    PaladinId id,
    std::size_t targetIndex
) {
    if (targetIndex >= team.size()) {
        return false;
    }

    int sourceIndex = FindMemberIndex(id);
    if (sourceIndex >= 0) {
        if (sourceIndex == static_cast<int>(targetIndex)) {
            return true;
        }

        Paladin* activePaladin = GetActivePaladin();
        std::swap(team[static_cast<std::size_t>(sourceIndex)], team[targetIndex]);

        auto activeIt = std::find(team.begin(), team.end(), activePaladin);
        if (activeIt != team.end()) {
            activeIndex = static_cast<int>(activeIt - team.begin());
        }
    } else {
        Paladin* newPaladin = nullptr;
        for (auto* p : roster) {
            if (p->GetPaladinId() == id) {
                newPaladin = p;
                break;
            }
        }
        if (!newPaladin) return false;

        Paladin* oldPaladin = team[targetIndex];
        newPaladin->SetPosition(oldPaladin->GetPosition());
        newPaladin->SetAimTarget(oldPaladin->GetAimTarget());
        team[targetIndex] = newPaladin;
    }

    NotifyObservers();
    return true;
}

void TeamManager::AddDepthRenderItems(std::vector<DepthRenderItem>& items) {
    if (team.empty()) return;
    
    Paladin* active = GetActivePaladin();
    for (auto* paladin : team) {
        if (paladin == active) {
            items.push_back({
                paladin->GetBoundingBox().y + paladin->GetBoundingBox().height,
                [paladin]() { paladin->Draw(); }
            });
        } else {
            items.push_back({
                paladin->GetBoundingBox().y + paladin->GetBoundingBox().height,
                [paladin]() { paladin->DrawInactive(); }
            });
        }
    }
    
    // Draw shared buffs slightly behind the active paladin
    if (active && !sharedBuffs.empty()) {
        items.push_back({
            active->GetBoundingBox().y + active->GetBoundingBox().height - 0.1f,
            [this]() { this->DrawBuffs(); }
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
    newActive->SetCurrentAimAngle(oldActive->GetCurrentAimAngle());
    newActive->SetTargetAimAngle(oldActive->GetTargetAimAngle());
    newActive->SetCurrentAimStrategy(oldActive->GetCurrentAimStrategy());
    newActive->SetFacingLeft(oldActive->IsFacingLeft());
    
    // Manage shared buffs
    for (auto& buff : sharedBuffs) {
        buff->OnRemove(oldActive);
        buff->OnApply(newActive);
    }
    oldActive->SetInvulnerable(false); // Failsafe cleanup
    
    if (oldActive->IsParrying()) {
        newActive->ChangeState(newActive->GetParryState());
    } else {
        newActive->ChangeState(newActive->GetIdleState());
    }
    
    newActive->TriggerSwapParryWindow();
    
    NotifyObservers();
}

void TeamManager::SwapCharacterToIndex(int targetIndex) {
    if (team.size() <= 1) return;
    if (targetIndex < 0 || targetIndex >= team.size()) return;
    if (activeIndex == targetIndex) return; // Already active
    
    if (team[targetIndex]->GetHealth() <= 0) return; // Do nothing if downed
    
    Paladin* oldActive = GetActivePaladin();
    if (!oldActive) return;
    
    activeIndex = targetIndex;
    Paladin* newActive = GetActivePaladin();
    
    newActive->SetPosition(oldActive->GetPosition());
    newActive->SetAimTarget(oldActive->GetAimTarget());
    newActive->SetCurrentAimAngle(oldActive->GetCurrentAimAngle());
    newActive->SetTargetAimAngle(oldActive->GetTargetAimAngle());
    newActive->SetCurrentAimStrategy(oldActive->GetCurrentAimStrategy());
    newActive->SetFacingLeft(oldActive->IsFacingLeft());
    
    // Manage shared buffs
    for (auto& buff : sharedBuffs) {
        buff->OnRemove(oldActive);
        buff->OnApply(newActive);
    }
    oldActive->SetInvulnerable(false); // Failsafe cleanup
    
    if (oldActive->IsParrying()) {
        newActive->ChangeState(newActive->GetParryState());
    } else {
        newActive->ChangeState(newActive->GetIdleState());
    }
    
    newActive->TriggerSwapParryWindow();
    NotifyObservers();
}

void TeamManager::SwapDueToDeath() {
    if (team.size() <= 1) return;
    
    Paladin* deadActive = GetActivePaladin();
    if (!deadActive) return;
    
    int attempts = 0;
    int nextIndex = activeIndex;
    bool foundAlive = false;
    
    while (attempts < team.size()) {
        nextIndex = (nextIndex + 1) % team.size();
        if (team[nextIndex]->GetHealth() > 0) {
            foundAlive = true;
            break;
        }
        attempts++;
    }
    
    if (!foundAlive) return;
    
    activeIndex = nextIndex;
    Paladin* newActive = GetActivePaladin();
    
    // Transfer position and aim target
    newActive->SetPosition(deadActive->GetPosition());
    newActive->SetAimTarget(deadActive->GetAimTarget());
    newActive->SetCurrentAimAngle(deadActive->GetCurrentAimAngle());
    newActive->SetTargetAimAngle(deadActive->GetTargetAimAngle());
    newActive->SetCurrentAimStrategy(deadActive->GetCurrentAimStrategy());
    newActive->SetFacingLeft(deadActive->IsFacingLeft());
    
    newActive->ChangeState(newActive->GetIdleState());
    
    NotifyObservers();
}

#include "raymath.h"
void TeamManager::Update(float deltaTime) {
    if (team.empty()) return;

    displayedQuintessence = Lerp(displayedQuintessence, currentQuintessence, 10.0f * deltaTime);

    // Check if current active paladin is dead is now handled by PlayerDownState deferred logic.
    Paladin* active = GetActivePaladin();

    if (!active->IsDoingUltimate()) {
        if (IsKeyPressed(KEY_TAB)) {
            SwapCharacter();
        } else if (IsKeyPressed(KEY_ONE)) {
            SwapCharacterToIndex(0);
        } else if (IsKeyPressed(KEY_TWO)) {
            SwapCharacterToIndex(1);
        } else if (IsKeyPressed(KEY_THREE)) {
            SwapCharacterToIndex(2);
        }
    }

    for (auto* paladin : team) {
        if (paladin == active) {
            paladin->Update(deltaTime);
        } else {
            paladin->UpdateInactive(deltaTime);
        }
    }
    
    // Update shared buffs
    for (auto it = sharedBuffs.begin(); it != sharedBuffs.end(); ) {
        (*it)->Update(deltaTime, active);
        if ((*it)->IsFinished()) {
            (*it)->OnRemove(active);
            it = sharedBuffs.erase(it);
        } else {
            ++it;
        }
    }

    // Tick ultimate cooldowns for ALL team members (active + benched)
    for (auto* paladin : team) {
        paladin->TickUltimateCooldown(deltaTime);
    }
}

void TeamManager::Draw() {
    if (team.empty()) return;
    
    Paladin* active = GetActivePaladin();
    for (auto* paladin : team) {
        if (paladin != active) {
            paladin->DrawInactive();
        }
    }
    active->Draw();
}

void TeamManager::DrawBuffs() {
    Paladin* active = GetActivePaladin();
    for (auto& buff : sharedBuffs) {
        buff->Draw(active);
    }
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

void TeamManager::AddQuintessence(float amount) {
    if (debugFastFuel) {
        amount *= 20.0f;
    }
    currentQuintessence += amount;
    if (currentQuintessence > maxQuintessence) {
        currentQuintessence = maxQuintessence;
    }
}

bool TeamManager::ConsumeQuintessence(float amount) {
    if (currentQuintessence < amount) {
        return false;
    }
    currentQuintessence -= amount;
    return true;
}
