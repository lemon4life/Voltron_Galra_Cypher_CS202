#include "Entities/Components/StatusComponent.h"
#include "Entities/Enemy.h"

void StatusComponent::AddEffect(EffectType type, float duration, float magnitude) {
    // Check if effect already exists to refresh it
    for (auto& mod : activeModifiers) {
        if (mod.type == type) {
            if (mod.duration < duration) {
                mod.duration = duration;
            }
            mod.magnitude = magnitude; // Update magnitude just in case
            return;
        }
    }
    
    // Otherwise add new
    activeModifiers.push_back({type, duration, 0.0f, magnitude});
}

bool StatusComponent::Update(float deltaTime, Enemy* owner) {
    bool isFrozen = false;
    
    for (auto it = activeModifiers.begin(); it != activeModifiers.end(); ) {
        it->duration -= deltaTime;
        
        if (it->duration <= 0.0f) {
            it = activeModifiers.erase(it);
            continue;
        }
        
        if (it->type == EffectType::BURN) {
            it->tickTimer += deltaTime;
            if (it->tickTimer >= 1.0f) {
                owner->TakeDamage(static_cast<int>(it->magnitude));
                it->tickTimer -= 1.0f;
            }
        } else if (it->type == EffectType::FREEZE) {
            isFrozen = true;
        }
        
        ++it;
    }
    
    if (isFrozen) {
        owner->SetCurrentVelocity({0.0f, 0.0f});
    }
    
    return isFrozen;
}

Color StatusComponent::GetStatusTint() const {
    bool hasBurn = false;
    for (const auto& mod : activeModifiers) {
        if (mod.type == EffectType::FREEZE) {
            return SKYBLUE; // Freeze takes priority
        }
        if (mod.type == EffectType::BURN) {
            hasBurn = true;
        }
    }
    return hasBurn ? RED : WHITE;
}

void StatusComponent::Clear() {
    activeModifiers.clear();
}
