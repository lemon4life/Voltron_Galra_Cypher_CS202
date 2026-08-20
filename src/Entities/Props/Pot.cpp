#include "Entities/Props/Pot.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/AudioManager.h"

HpPot::HpPot(Vector2 pos) : Pot(pos, "pot_hp") {}

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

ExPot::ExPot(Vector2 pos) : Pot(pos, "pot_ex") {}

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

QuintPot::QuintPot(Vector2 pos) : Pot(pos, "pot_quint") {}

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
