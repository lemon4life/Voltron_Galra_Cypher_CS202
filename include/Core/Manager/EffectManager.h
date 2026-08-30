#pragma once

#include "raylib.h"

#include <vector>

struct ImpactEffect {
    Vector2 position;
    float lifetime;
    float maxLifetime;
    int currentFrame;
    int numFrames;
    Texture2D texture;
    bool drawBehind;
    Color tint = WHITE;
    Vector2 origin = { 0.0f, 0.0f };
};

class LevelManager;

class EffectManager {
private:
    std::vector<ImpactEffect> activeEffects;
    Texture2D bulletImpactTexture = {};
    const LevelManager* levelManager = nullptr;

public:
    /// Connects this component to the managers and services it needs at runtime.
    void Configure(const LevelManager& level) { levelManager = &level; }
    /// Initializes the resources and collaborators required before this component can run.
    void Initialize();
    /// Releases resources owned by this component and leaves it safe to destroy.
    void Shutdown();
    /// Updates the stored bullet impact texture.
    void SetBulletImpactTexture(Texture2D texture) {
        bulletImpactTexture = texture;
    }

    /// Adds effect.
    void AddEffect(
        Vector2 position,
        Texture2D texture,
        int frames,
        float lifetime,
        bool drawBehind = false,
        Color tint = WHITE
    );
    /// Adds anchored effect.
    void AddAnchoredEffect(
        Vector2 position,
        Texture2D texture,
        int frames,
        float lifetime,
        Vector2 origin,
        bool drawBehind = false,
        Color tint = WHITE
    );
    /// Adds impact effect.
    void AddImpactEffect(Vector2 position);
    /// Spawns impact.
    void SpawnImpact(
        Vector2 position,
        Vector2 velocity,
        Color color,
        int count
    );
    /// Spawns parry sparks.
    void SpawnParrySparks(Vector2 position, int count);
    /// Spawns damage number.
    void SpawnDamageNumber(Vector2 position, int damage);
    /// Spawns dash trail.
    void SpawnDashTrail(
        Vector2 position,
        Rectangle source,
        Texture2D texture,
        float rotation,
        bool flipX
    );
    /// Adds corpse.
    void AddCorpse(
        Vector2 position,
        Texture2D texture,
        bool facingLeft,
        Vector2 slideVelocity
    );
    /// Advances this component's state for the current frame.
    void Update(float deltaTime);
    /// Renders this component using its current state and visual resources.
    void Draw(bool background) const;
    /// Renders particles.
    void DrawParticles() const;
    /// Clears session.
    void ClearSession();
    /// Returns the current active effect count.
    std::size_t GetActiveEffectCount() const { return activeEffects.size(); }
    /// Returns the current active effect capacity.
    std::size_t GetActiveEffectCapacity() const {
        return activeEffects.capacity();
    }
};
