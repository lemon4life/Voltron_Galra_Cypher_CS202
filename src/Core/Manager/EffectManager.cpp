#include "Core/Manager/EffectManager.h"

#include "Core/Manager/DecalManager.h"
#include "Core/Manager/ParticleManager.h"

#include <algorithm>

namespace {
constexpr std::size_t MAX_ACTIVE_EFFECTS = 512;
}

void EffectManager::Initialize() {
    ParticleManager::GetInstance().Initialize();
}

void EffectManager::Shutdown() {
    ClearSession();
    ParticleManager::GetInstance().Shutdown();
}

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

void EffectManager::AddAnchoredEffect(
    Vector2 position,
    Texture2D texture,
    int frames,
    float lifetime,
    Vector2 origin,
    bool drawBehind,
    Color tint
) {
    if (frames <= 0 || lifetime <= 0.0f) return;
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

void EffectManager::AddImpactEffect(Vector2 position) {
    AddEffect(position, bulletImpactTexture, 4, 0.2f);
}

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

void EffectManager::SpawnParrySparks(Vector2 position, int count) {
    ParticleManager::GetInstance().SpawnParrySparks(position, count);
}

void EffectManager::SpawnDamageNumber(Vector2 position, int damage) {
    ParticleManager::GetInstance().SpawnDamageNumber(position, damage);
}

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

void EffectManager::Update(float deltaTime) {
    for (auto iterator = activeEffects.begin();
         iterator != activeEffects.end();) {
        iterator->lifetime -= deltaTime;
        if (iterator->lifetime <= 0.0f) {
            iterator = activeEffects.erase(iterator);
            continue;
        }

        if (iterator->texture.id != 0) {
            float progress = 1.0f -
                iterator->lifetime / iterator->maxLifetime;
            iterator->currentFrame = std::min(
                iterator->numFrames - 1,
                (int)(progress * iterator->numFrames)
            );
        }
        ++iterator;
    }

    ParticleManager::GetInstance().Update(deltaTime);
    DecalManager::GetInstance().Update(deltaTime, levelManager);
}

void EffectManager::Draw(bool background) const {
    if (background) {
        DecalManager::GetInstance().Draw();
    }
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

void EffectManager::DrawParticles() const {
    ParticleManager::GetInstance().Draw();
}

void EffectManager::ClearSession() {
    decltype(activeEffects){}.swap(activeEffects);
    ParticleManager::GetInstance().Clear();
    DecalManager::GetInstance().Clear();
}
