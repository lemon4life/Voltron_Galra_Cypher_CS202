#pragma once
#include "raylib.h"
#include <vector>

#include "Core/Visuals/IParticle.h"
#include <memory>

class SpriteParticle : public IParticle {
public:
    // Transform
    Vector2 position;
    Vector2 velocity;

    // Appearance
    Color color;
    float size;

    // Lifecycle
    float lifeSpan;
    float lifeRemaining;

    // Sprite ghosting (optional — if texture.id == 0, draw as primitive shape)
    Texture2D texture;
    Rectangle sourceRect;
    float rotation;

    // If true, the silhouette shader is used: sprite alpha determines shape,
    // but all colored pixels are replaced by `color` (e.g. solid blue ghost).
    bool silhouette;
    
    // We need a reference to the shader for drawing, or we can fetch it globally.
    // For simplicity, we'll pass it if needed, or use the global shader from ParticleManager.

    SpriteParticle(Vector2 pos, Vector2 vel, Color col, float sz, float life, 
                   Texture2D tex = {0}, Rectangle srcRect = {0}, float rot = 0.0f, bool sil = false);

    void Update(float deltaTime) override;
    void Draw() const override;
    bool IsDead() const override;
};

class ParticleManager {
private:
    std::vector<std::unique_ptr<IParticle>> activeParticles;

    Shader silhouetteShader; // Replaces sprite RGB with a solid fill color

    ParticleManager();
    ~ParticleManager();

public:
    static ParticleManager& GetInstance();

    // Delete copy/move for singleton
    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;
    ParticleManager(ParticleManager&&) = delete;
    ParticleManager& operator=(ParticleManager&&) = delete;

    // Must be called once after InitWindow() to load the silhouette shader.
    void Initialize();
    void Shutdown();

    // --- Emission API ---
    void Emit(Vector2 position, Vector2 velocity, Color color, float size, float lifeSpan);

    // silhouette=true uses the solid-fill shader instead of a textured tint
    void EmitSprite(Vector2 position, Vector2 velocity, Texture2D texture, Rectangle sourceRect,
                    float rotation, float size, float lifeSpan, Color tint,
                    bool silhouette = false);

    // --- Update & Draw ---
    void Update(float deltaTime);
    void Draw();

    void Clear();
    std::size_t GetActiveCount() const { return activeParticles.size(); }
    std::size_t GetCapacity() const { return activeParticles.capacity(); }
    
    Shader GetSilhouetteShader() const { return silhouetteShader; }

    // --- Combat Emitters ---
    void SpawnDashTrail(Vector2 pos, Rectangle sourceRect, Texture2D texture, float rotation, bool flipX);
    void SpawnParrySparks(Vector2 pos, int count);
    void SpawnImpact(Vector2 pos, Vector2 projectileVelocity, Color color, int count);
    void SpawnDamageNumber(Vector2 pos, int damage);
};
