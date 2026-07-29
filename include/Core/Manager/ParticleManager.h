#pragma once
#include "raylib.h"
#include <vector>

struct Particle {
    // Transform
    Vector2 position;
    Vector2 velocity;

    // Appearance
    Color color;
    float size;

    // Lifecycle
    float lifeSpan;
    float lifeRemaining;
    bool active;

    // Sprite ghosting (optional — if texture.id == 0, draw as primitive shape)
    Texture2D texture;
    Rectangle sourceRect;
    float rotation;

    // If true, the silhouette shader is used: sprite alpha determines shape,
    // but all colored pixels are replaced by `color` (e.g. solid blue ghost).
    bool silhouette;
};

class ParticleManager {
private:
    static constexpr int POOL_SIZE = 1000;

    std::vector<Particle> pool;
    int nextSearchIndex;

    Shader silhouetteShader; // Replaces sprite RGB with a solid fill color

    ParticleManager();
    ~ParticleManager();

    int GetNextAvailableIndex();

public:
    static ParticleManager& GetInstance();

    // Delete copy/move for singleton
    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;
    ParticleManager(ParticleManager&&) = delete;
    ParticleManager& operator=(ParticleManager&&) = delete;

    // Must be called once after InitWindow() to load the silhouette shader.
    void Initialize();

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

    // --- Combat Emitters ---
    void SpawnDashTrail(Vector2 pos, Rectangle sourceRect, Texture2D texture, float rotation, bool flipX);
    void SpawnParrySparks(Vector2 pos, int count);
    void SpawnImpact(Vector2 pos, Vector2 projectileVelocity, Color color, int count);
};
