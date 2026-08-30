#include "Entities/Props/Pot.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/AudioManager.h"

/// Creates a HpPot instance from the supplied configuration.
HpPot::HpPot(Vector2 pos) : Pot(pos, "pot_hp") {}

/// Handles the consume event.
void HpPot::OnConsume(TeamManager* team) {
    if (isConsumed || !team) return;
    isConsumed = true;
    AudioManager::GetInstance().PlaySoundEffect("fx_get_buff");
    for (Paladin* p : team->GetTeam()) {
        if (p->GetHealth() > 0) {
            int healAmount = p->GetMaxHealth() * 0.2f;
            p->Heal(healAmount);
            p->AddAttachedEffect(AssetManager::GetInstance().GetTexture("HP_effect"), 8, 0.5f);
        }
    }
}

/// Creates a ExPot instance from the supplied configuration.
ExPot::ExPot(Vector2 pos) : Pot(pos, "pot_ex") {}

/// Handles the consume event.
void ExPot::OnConsume(TeamManager* team) {
    if (isConsumed || !team) return;
    isConsumed = true;
    AudioManager::GetInstance().PlaySoundEffect("fx_get_buff");
    for (Paladin* p : team->GetTeam()) {
        if (p->GetHealth() > 0) {
            float exAmount = p->GetMaxExEnergy() * 0.3f;
            p->AddExEnergy(exAmount);
            p->AddAttachedEffect(AssetManager::GetInstance().GetTexture("Ex_effect"), 8, 0.5f);
        }
    }
}

/// Creates a QuintPot instance from the supplied configuration.
QuintPot::QuintPot(Vector2 pos) : Pot(pos, "pot_quint") {}

/// Handles the consume event.
void QuintPot::OnConsume(TeamManager* team) {
    if (isConsumed || !team) return;
    isConsumed = true;
    AudioManager::GetInstance().PlaySoundEffect("fx_get_buff");
    team->AddQuintessence(20.0f);
    
    // Apply effect to active paladin
    Paladin* active = team->GetActivePaladin();
    if (active && active->GetHealth() > 0) {
        active->AddAttachedEffect(AssetManager::GetInstance().GetTexture("Quint_effect"), 8, 0.5f);
    }
}
