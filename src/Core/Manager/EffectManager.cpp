#include "Core/Manager/EffectManager.h"

#include "Core/Manager/DecalManager.h"
#include "Core/Manager/ParticleManager.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr std::size_t MAX_ACTIVE_EFFECTS = 512;
}

/// Initializes the resources and collaborators required before this component can run.
void EffectManager::Initialize() {
    ParticleManager::GetInstance().Initialize();
}

/// Releases resources owned by this component and leaves it safe to destroy.
void EffectManager::Shutdown() {
    ClearSession();
    ParticleManager::GetInstance().Shutdown();
}

/// Adds effect.
void EffectManager::AddEffect(
    Vector2 position,
    Texture2D texture,
    int frames,
    float lifetime,
    bool drawBehind,
    Color tint
) {
    if (frames <= 0 || lifetime <= 0.0f ||
        activeEffects.size() >= MAX_ACTIVE_EFFECTS) return;
    AddAnchoredEffect(
        position,
        texture,
        frames,
        lifetime,
        {
            (float)texture.width / frames * 0.5f,
            (float)texture.height * 0.5f
        },
        drawBehind,
        tint
    );
}

/// Adds anchored effect.
void EffectManager::AddAnchoredEffect(
    Vector2 position,
    Texture2D texture,
    int frames,
    float lifetime,
    Vector2 origin,
    bool drawBehind,
    Color tint
) {
    if (frames <= 0 || !std::isfinite(lifetime) || lifetime <= 0.0f ||
        !std::isfinite(position.x) || !std::isfinite(position.y) ||
        texture.id == 0 || texture.width <= 0 || texture.height <= 0 ||
        texture.width % frames != 0 ||
        activeEffects.size() >= MAX_ACTIVE_EFFECTS) {
        return;
    }
    activeEffects.push_back({
        position,
        lifetime,
        lifetime,
        0,
        frames,
        texture,
        drawBehind,
        tint,
        origin
    });
}

/// Adds impact effect.
void EffectManager::AddImpactEffect(Vector2 position) {
    AddEffect(position, bulletImpactTexture, 4, 0.2f);
}

/// Spawns impact.
void EffectManager::SpawnImpact(
    Vector2 position,
    Vector2 velocity,
    Color color,
    int count
) {
    ParticleManager::GetInstance().SpawnImpact(
        position, velocity, color, count
    );
}

/// Spawns parry sparks.
void EffectManager::SpawnParrySparks(Vector2 position, int count) {
    ParticleManager::GetInstance().SpawnParrySparks(position, count);
}

/// Spawns damage number.
void EffectManager::SpawnDamageNumber(Vector2 position, int damage) {
    ParticleManager::GetInstance().SpawnDamageNumber(position, damage);
}

/// Spawns dash trail.
void EffectManager::SpawnDashTrail(
    Vector2 position,
    Rectangle source,
    Texture2D texture,
    float rotation,
    bool flipX
) {
    ParticleManager::GetInstance().SpawnDashTrail(
        position, source, texture, rotation, flipX
    );
}

/// Adds corpse.
void EffectManager::AddCorpse(
    Vector2 position,
    Texture2D texture,
    bool facingLeft,
    Vector2 slideVelocity
) {
    DecalManager::GetInstance().AddCorpse(
        position, texture, facingLeft, slideVelocity
    );
}

/// Advances this component's state for the current frame.
void EffectManager::Update(float deltaTime) {
    for (std::size_t index = 0; index < activeEffects.size();) {
        ImpactEffect& effect = activeEffects[index];
        effect.lifetime -= deltaTime;
        if (effect.lifetime <= 0.0f) {
            effect = std::move(activeEffects.back());
            activeEffects.pop_back();
            continue;
        }

        if (effect.texture.id != 0) {
            float progress = 1.0f -
                effect.lifetime / effect.maxLifetime;
            effect.currentFrame = std::min(
                effect.numFrames - 1,
                (int)(progress * effect.numFrames)
            );
        }
        ++index;
    }

    ParticleManager::GetInstance().Update(deltaTime);
    DecalManager::GetInstance().Update(deltaTime, levelManager);
}

/// Renders this component using its current state and visual resources.
void EffectManager::Draw(bool background) const {
    for (const ImpactEffect& effect : activeEffects) {
        if (effect.drawBehind != background || effect.texture.id == 0) {
            continue;
        }

        float frameWidth =
            (float)effect.texture.width / effect.numFrames;
        float frameHeight = (float)effect.texture.height;
        Rectangle source = {
            effect.currentFrame * frameWidth,
            0.0f,
            frameWidth,
            frameHeight
        };
        Rectangle destination = {
            effect.position.x,
            effect.position.y,
            frameWidth,
            frameHeight
        };
        DrawTexturePro(
            effect.texture,
            source,
            destination,
            effect.origin,
            0.0f,
            effect.tint
        );
    }
}

/// Renders particles.
void EffectManager::DrawParticles() const {
    ParticleManager::GetInstance().Draw();
}

/// Clears session.
void EffectManager::ClearSession() {
    activeEffects.clear();
    ParticleManager::GetInstance().Clear();
    DecalManager::GetInstance().Clear();
}
