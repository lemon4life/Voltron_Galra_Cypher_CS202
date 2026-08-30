#pragma once
#include "raylib.h"
#include <vector>

#include "Core/Visuals/IParticle.h"
#include "Core/Visuals/DamageTextParticle.h"

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

    /// Creates a SpriteParticle instance from the supplied configuration.
    SpriteParticle(Vector2 pos, Vector2 vel, Color col, float sz, float life, 
                   Texture2D tex = {0}, Rectangle srcRect = {0}, float rot = 0.0f, bool sil = false);

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() const override;
    /// Reports whether the dead condition is satisfied.
    bool IsDead() const override;
};

// Design Pattern - Singleton:
// ParticleManager centrally owns active polymorphic particles and damage text;
// GetInstance prevents separate particle collections across gameplay systems.
class ParticleManager {
private:
    std::vector<SpriteParticle> activeParticles;
    std::vector<DamageTextParticle> damageTextParticles;

    Shader silhouetteShader; // Replaces sprite RGB with a solid fill color

    /// Creates a ParticleManager instance from the supplied configuration.
    ParticleManager();
    /// Releases resources owned by this ParticleManager instance.
    ~ParticleManager();

public:
    /// Returns the process-wide singleton instance of this manager.
    static ParticleManager& GetInstance();

    // Delete copy/move for singleton
    /// Creates a ParticleManager instance from the supplied configuration.
    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;
    /// Creates a ParticleManager instance from the supplied configuration.
    ParticleManager(ParticleManager&&) = delete;
    ParticleManager& operator=(ParticleManager&&) = delete;

    // Must be called once after InitWindow() to load the silhouette shader.
    /// Initializes the resources and collaborators required before this component can run.
    void Initialize();
    /// Releases resources owned by this component and leaves it safe to destroy.
    void Shutdown();

    // --- Emission API ---
    /// Implements the emit behavior for this component.
    void Emit(Vector2 position, Vector2 velocity, Color color, float size, float lifeSpan);

    // silhouette=true uses the solid-fill shader instead of a textured tint
    /// Emits sprite.
    void EmitSprite(Vector2 position, Vector2 velocity, Texture2D texture, Rectangle sourceRect,
                    float rotation, float size, float lifeSpan, Color tint,
                    bool silhouette = false);

    // --- Update & Draw ---
    /// Advances this component's state for the current frame.
    void Update(float deltaTime);
    /// Renders this component using its current state and visual resources.
    void Draw();

    /// Removes all runtime entries owned by this component and resets transient state.
    void Clear();
    /// Returns the current active count.
    std::size_t GetActiveCount() const {
        return activeParticles.size() + damageTextParticles.size();
    }
    /// Returns the current capacity.
    std::size_t GetCapacity() const {
        return activeParticles.capacity() + damageTextParticles.capacity();
    }
    
    /// Returns the current silhouette shader.
    Shader GetSilhouetteShader() const { return silhouetteShader; }

    // --- Combat Emitters ---
    /// Spawns dash trail.
    void SpawnDashTrail(Vector2 pos, Rectangle sourceRect, Texture2D texture, float rotation, bool flipX);
    /// Spawns parry sparks.
    void SpawnParrySparks(Vector2 pos, int count);
    /// Spawns impact.
    void SpawnImpact(Vector2 pos, Vector2 projectileVelocity, Color color, int count);
    /// Spawns damage number.
    void SpawnDamageNumber(Vector2 pos, int damage);
};
