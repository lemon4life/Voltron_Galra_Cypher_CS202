#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "raylib.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/InputManager.h"
#include "Core/Constants.h"
#include <algorithm>
#include <cmath>

/// Creates a TeamManager instance from the supplied configuration.
TeamManager::TeamManager()
    : activeIndex(0),
      sharedArmor(0),
      maxSharedArmor(0),
      autoStrategy(std::make_unique<AutoAimStrategy>()),
      mouseStrategy(std::make_unique<MouseAimStrategy>())
{
}

/// Releases resources owned by this TeamManager instance.
TeamManager::~TeamManager() = default;

/// Adds member.
void TeamManager::AddMember(std::unique_ptr<Paladin> paladin) {
    if (!paladin) return;
    Paladin* member = paladin.get();
    roster.push_back(std::move(paladin));
    if (team.size() < 3) {
        team.push_back(member);
    }
    member->SetTeamManager(this);
}

/// Refreshes aim strategies.
void TeamManager::RefreshAimStrategies() {
    bool useAuto = Constants::isAutoAimEnabled ||
                   InputManager::GetMode() == InputMode::KEYBOARD_ONLY;
    IAimStrategy* chosen = useAuto ? autoStrategy.get() : mouseStrategy.get();
    for (auto* p : team) {
        p->SetCurrentAimStrategy(chosen);
    }
}

/// Returns the current active paladin.
Paladin* TeamManager::GetActivePaladin() const {
    if (team.empty() || activeIndex < 0 ||
        activeIndex >= static_cast<int>(team.size())) {
        return nullptr;
    }
    return team[static_cast<std::size_t>(activeIndex)];
}

/// Captures the whole roster independently from the three selected team slots.
SavedTeamState TeamManager::CaptureCheckpointState() const {
    SavedTeamState saved;
    saved.activeIndex = activeIndex;
    saved.sharedArmor = sharedArmor;
    saved.maxSharedArmor = maxSharedArmor;
    saved.quintessence = currentQuintessence;
    saved.coins = coins;
    for (const std::unique_ptr<Paladin>& paladin : roster) {
        if (paladin) saved.roster.push_back(paladin->CaptureCheckpointState());
    }
    for (const Paladin* paladin : team) {
        if (paladin) {
            saved.selectedSlots.push_back(
                static_cast<int>(paladin->GetPaladinId())
            );
        }
    }
    return saved;
}

/// Restores stable character values and rebuilds the non-owning selected-team view.
bool TeamManager::RestoreCheckpointState(const SavedTeamState& saved) {
    if (saved.roster.empty() || saved.selectedSlots.empty()) return false;

    for (const SavedPaladinState& paladinState : saved.roster) {
        auto match = std::find_if(
            roster.begin(),
            roster.end(),
            [&](const std::unique_ptr<Paladin>& paladin) {
                return paladin && static_cast<int>(paladin->GetPaladinId()) ==
                    paladinState.id;
            }
        );
        if (match == roster.end()) return false;
        (*match)->RestoreCheckpointState(paladinState);
    }

    std::vector<Paladin*> restoredTeam;
    restoredTeam.reserve(saved.selectedSlots.size());
    for (int savedId : saved.selectedSlots) {
        auto match = std::find_if(
            roster.begin(),
            roster.end(),
            [savedId](const std::unique_ptr<Paladin>& paladin) {
                return paladin && static_cast<int>(paladin->GetPaladinId()) ==
                    savedId;
            }
        );
        if (match == roster.end()) return false;
        restoredTeam.push_back(match->get());
    }

    Paladin* oldActive = GetActivePaladin();
    for (std::unique_ptr<IBuff>& buff : sharedBuffs) {
        if (buff) buff->OnRemove(oldActive);
    }
    sharedBuffs.clear();
    team = std::move(restoredTeam);
    activeIndex = std::clamp(
        saved.activeIndex,
        0,
        static_cast<int>(team.size()) - 1
    );
    sharedArmor = std::max(0, saved.sharedArmor);
    maxSharedArmor = std::max(sharedArmor, saved.maxSharedArmor);
    currentQuintessence = std::clamp(
        saved.quintessence,
        0.0f,
        maxQuintessence
    );
    displayedQuintessence = currentQuintessence;
    coins = std::max(0, saved.coins);
    isSpawning = false;
    spawnAnimTimer = 0.0f;
    RefreshAimStrategies();
    NotifyObservers();
    return true;
}

/// Resets for new game.
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
    currentQuintessence = 0.0f;
    displayedQuintessence = 0.0f;
    coins = 0;

    for (const std::unique_ptr<Paladin>& ownedPaladin : roster) {
        Paladin* paladin = ownedPaladin.get();
        paladin->SetPosition(spawnPosition);
        paladin->ResetStats();
        paladin->SetAimTarget(spawnPosition);
    }
    
    StartSpawnAnimation();

    NotifyObservers();
}

/// Searches for member index.
int TeamManager::FindMemberIndex(PaladinId id) const {
    for (std::size_t index = 0; index < team.size(); ++index) {
        if (team[index] && team[index]->GetPaladinId() == id) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

/// Assigns paladin to slot.
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
        for (const std::unique_ptr<Paladin>& ownedPaladin : roster) {
            Paladin* p = ownedPaladin.get();
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

/// Adds depth render items.
void TeamManager::AddDepthRenderItems(std::vector<DepthRenderItem>& items) {
    if (team.empty()) return;
    
    Paladin* active = GetActivePaladin();
    for (auto* paladin : team) {
        if (paladin == active) {
            items.push_back({
                paladin->GetBoundingBox().y + paladin->GetBoundingBox().height,
                [this, paladin]() { 
                    if (!isSpawning || spawnAnimTimer >= 0.3f) {
                        paladin->Draw();
                    }
                }
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
            [this]() { DrawBuffs(); }
        });
    }

    if (isSpawning) {
        items.push_back({
            active->GetBoundingBox().y + active->GetBoundingBox().height + 0.1f,
            [this, active]() {
                int frame = (int)(spawnAnimTimer / 0.1f);
                if (frame > 4) frame = 4;
                Texture2D light = AssetManager::GetInstance().GetTexture("AppearLight");
                Texture2D smoke = AssetManager::GetInstance().GetTexture("AppearSmoke");
                
                Vector2 pos = active->GetPosition();
                float footY = active->GetBoundingBox().y + active->GetBoundingBox().height;

                if (smoke.id != 0) {
                    float fw = smoke.width / 5.0f;
                    Rectangle src = { frame * fw, 0.0f, fw, (float)smoke.height };
                    Rectangle dest = { pos.x, footY, fw, (float)smoke.height };
                    DrawTexturePro(smoke, src, dest, { fw / 2.0f, (float)smoke.height }, 0.0f, WHITE);
                }

                if (light.id != 0) {
                    float fw = light.width / 5.0f;
                    Rectangle src = { frame * fw, 0.0f, fw, (float)light.height };
                    Rectangle dest = { pos.x, footY, fw, (float)light.height };
                    DrawTexturePro(light, src, dest, { fw / 2.0f, (float)light.height }, 0.0f, WHITE);
                }
            }
        });
    }
}

/// Starts spawn animation.
void TeamManager::StartSpawnAnimation() {
    isSpawning = true;
    spawnAnimTimer = 0.0f;
    AudioManager::GetInstance().PlaySoundEffect("fx_show_up");
}

/// Swaps character.
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
    
    AudioManager::GetInstance().PlaySoundEffect("fx_switch_character");
    
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

/// Swaps character to index.
void TeamManager::SwapCharacterToIndex(int targetIndex) {
    if (team.size() <= 1) return;
    if (targetIndex < 0 || targetIndex >= team.size()) return;
    if (activeIndex == targetIndex) return; // Already active
    
    if (team[targetIndex]->GetHealth() <= 0) return; // Do nothing if downed
    
    Paladin* oldActive = GetActivePaladin();
    if (!oldActive) return;
    
    activeIndex = targetIndex;
    Paladin* newActive = GetActivePaladin();
    
    AudioManager::GetInstance().PlaySoundEffect("fx_switch_character");
    
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

/// Swaps due to death.
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
/// Updates team-wide input and shared status while only the selected Paladin
/// receives active movement/combat updates. Benched members still advance their
/// inactive state and Ultimate cooldowns so swapping preserves consistent timing.
void TeamManager::Update(float deltaTime) {
    if (team.empty()) return;

    if (isSpawning) {
        spawnAnimTimer += deltaTime;
        if (spawnAnimTimer >= 0.5f) {
            isSpawning = false;
        }
        return; // Prevent input and movement while spawning
    }

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

    NotifyObservers();
}

/// Renders this component using its current state and visual resources.
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

/// Renders buffs.
void TeamManager::DrawBuffs() {
    Paladin* active = GetActivePaladin();
    for (auto& buff : sharedBuffs) {
        buff->Draw(active);
    }
}

/// Notifies observers.
void TeamManager::NotifyObservers() {
    if (team.empty()) return;
    
    for (std::size_t i = 0; i < team.size(); ++i) {
        Paladin* p = team[i];
        if (!p) continue;

        PlayerStatsSnapshot stats;
        stats.slotIndex = static_cast<int>(i);
        stats.health = p->GetHealth();
        stats.maxHealth = p->GetMaxHealth();
        stats.displayedHp = p->GetDisplayedHp();
        stats.ghostHp = p->GetGhostHp();
        stats.exEnergy = p->GetExEnergy();
        stats.displayedEx = p->GetDisplayedExEnergy();
        stats.maxEx = p->GetMaxExEnergy();
        stats.skillCost = p->GetSkillCost();
        stats.isDowned = (p->GetHealth() <= 0);

        for (auto* observer : observers) {
            if (observer) {
                observer->OnPlayerStatsChanged(stats, static_cast<int>(i));
            }
        }
    }

    TeamStatsSnapshot teamStats;
    teamStats.activeIndex = activeIndex;
    teamStats.sharedArmor = sharedArmor;
    teamStats.maxSharedArmor = maxSharedArmor;
    teamStats.currentQuintessence = currentQuintessence;
    teamStats.displayedQuintessence = displayedQuintessence;
    teamStats.maxQuintessence = maxQuintessence;

    for (auto* observer : observers) {
        if (observer) {
            observer->OnTeamStatsChanged(teamStats);
        }
    }
}

/// Reports whether the team dead condition is satisfied.
bool TeamManager::IsTeamDead() const {
    for (auto* paladin : team) {
        if (paladin->GetHealth() > 0) return false;
    }
    return true;
}

/// Adds quintessence.
void TeamManager::AddQuintessence(float amount) {
    if (!std::isfinite(amount) || amount <= 0.0f) return;
    if (debugFastFuel) {
        amount *= 20.0f;
    }
    currentQuintessence += amount;
    if (currentQuintessence > maxQuintessence) {
        currentQuintessence = maxQuintessence;
    }
    NotifyObservers();
}

/// Consumes and returns quintessence.
bool TeamManager::ConsumeQuintessence(float amount) {
    if (!std::isfinite(amount) || amount <= 0.0f) return false;
    if (currentQuintessence < amount) {
        return false;
    }
    currentQuintessence -= amount;
    NotifyObservers();
    return true;
}
