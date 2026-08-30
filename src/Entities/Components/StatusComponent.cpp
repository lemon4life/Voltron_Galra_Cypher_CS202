#include "Entities/Components/StatusComponent.h"
#include "Entities/Enemy.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/ParticleManager.h"

/// Adds effect.
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

/// Reports whether this component has effect.
bool StatusComponent::HasEffect(EffectType type) const {
    for (const auto& mod : activeModifiers) {
        if (mod.type == type) return true;
    }
    return false;
}

/// Advances this component's state for the current frame.
bool StatusComponent::Update(float deltaTime, Enemy* owner) {
    bool isFrozen = false;
    
    for (auto it = activeModifiers.begin(); it != activeModifiers.end(); ) {
        it->duration -= deltaTime;
        
        if (it->duration <= 0.0f) {
            if (it->type == EffectType::FREEZE) {
                AudioManager::GetInstance().PlaySoundEffect("fx_ice_explode");
                Texture2D baseTex = AssetManager::GetInstance().GetTexture("Freeze_Base");
                if (baseTex.id != 0) {
                    Rectangle src = { 0, 0, (float)baseTex.width, (float)baseTex.height };
                    float size = (owner->GetEnemyType() == EnemyType::BOSS) ? baseTex.width * 2.0f : baseTex.width;
                    Vector2 spawnPos = owner->GetRenderFootPosition();
                    ParticleManager::GetInstance().EmitSprite(
                        spawnPos,
                        {0, 0},
                        baseTex,
                        src,
                        0.0f,
                        size,
                        1.5f,
                        WHITE,
                        false
                    );
                }
            }
            it = activeModifiers.erase(it);
            continue;
        }
        
        if (it->type == EffectType::BURN || it->type == EffectType::POISON) {
            it->tickTimer += deltaTime;
            if (it->tickTimer >= 1.0f) {
                owner->TakeDamage(static_cast<int>(it->magnitude));
                it->tickTimer -= 1.0f;
            }
        } else if (it->type == EffectType::FREEZE || it->type == EffectType::DIZZY) {
            isFrozen = true;
        }
        
        ++it;
    }
    
    if (isFrozen) {
        owner->SetCurrentVelocity({0.0f, 0.0f});
    }
    
    return isFrozen;
}

/// Returns the current status tint.
Color StatusComponent::GetStatusTint() const {
    bool hasBurn = false;
    bool hasPoison = false;
    for (const auto& mod : activeModifiers) {
        if (mod.type == EffectType::FREEZE) {
            return SKYBLUE; // Freeze takes priority
        }
        if (mod.type == EffectType::DIZZY) {
            return LIGHTGRAY;
        }
        if (mod.type == EffectType::BURN) {
            hasBurn = true;
        }
        if (mod.type == EffectType::POISON) {
            hasPoison = true;
        }
    }
    if (hasBurn) return RED;
    if (hasPoison) return GREEN;
    return WHITE;
}

/// Removes all runtime entries owned by this component and resets transient state.
void StatusComponent::Clear() {
    activeModifiers.clear();
}
