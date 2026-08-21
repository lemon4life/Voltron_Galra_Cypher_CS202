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
    void Configure(const LevelManager& level) { levelManager = &level; }
    void Initialize();
    void Shutdown();
    void SetBulletImpactTexture(Texture2D texture) {
        bulletImpactTexture = texture;
    }

    void AddEffect(
        Vector2 position,
        Texture2D texture,
        int frames,
        float lifetime,
        bool drawBehind = false,
        Color tint = WHITE
    );
    void AddAnchoredEffect(
        Vector2 position,
        Texture2D texture,
        int frames,
        float lifetime,
        Vector2 origin,
        bool drawBehind = false,
        Color tint = WHITE
    );
    void AddImpactEffect(Vector2 position);
    void SpawnImpact(
        Vector2 position,
        Vector2 velocity,
        Color color,
        int count
    );
    void SpawnParrySparks(Vector2 position, int count);
    void SpawnDamageNumber(Vector2 position, int damage);
    void SpawnDashTrail(
        Vector2 position,
        Rectangle source,
        Texture2D texture,
        float rotation,
        bool flipX
    );
    void AddCorpse(
        Vector2 position,
        Texture2D texture,
        bool facingLeft,
        Vector2 slideVelocity
    );
    void Update(float deltaTime);
    void Draw(bool background) const;
    void DrawParticles() const;
    void ClearSession();
};
