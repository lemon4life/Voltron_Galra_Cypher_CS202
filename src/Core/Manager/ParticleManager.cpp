#include "Core/Manager/ParticleManager.h"
#include "raymath.h"
#include <cmath>

// ─── Constructor ────────────────────────────────────────────────────────────

ParticleManager::ParticleManager() : nextSearchIndex(0), silhouetteShader({ 0 }) {
    pool.resize(POOL_SIZE);
    for (auto& p : pool) {
        p.active     = false;
        p.texture    = { 0 };
        p.silhouette = false;
    }
}

ParticleManager::~ParticleManager() {
    if (silhouetteShader.id != 0) UnloadShader(silhouetteShader);
}

void ParticleManager::Initialize() {
    // Fragment shader: uses the sprite's alpha for the shape, but replaces
    // all RGB with the tint color (fragColor). This produces a solid-fill silhouette.
    static const char* fragSrc = R"(
        #version 330
        in vec2 fragTexCoord;
        in vec4 fragColor;
        out vec4 finalColor;
        uniform sampler2D texture0;
        void main() {
            float alpha = texture(texture0, fragTexCoord).a;
            if (alpha < 0.05) discard;
            finalColor = vec4(fragColor.rgb, alpha * fragColor.a);
        }
    )";
    // NULL vertex shader = use Raylib's built-in default
    silhouetteShader = LoadShaderFromMemory(NULL, fragSrc);
}

// ─── Singleton ───────────────────────────────────────────────────────────────

ParticleManager& ParticleManager::GetInstance() {
    static ParticleManager instance;
    return instance;
}

// ─── Object Pool Helper ──────────────────────────────────────────────────────

int ParticleManager::GetNextAvailableIndex() {
    // Round-robin scan starting from our hint index for O(n) worst case
    // but typically O(1) amortized when particles expire naturally.
    for (int i = 0; i < POOL_SIZE; ++i) {
        int idx = (nextSearchIndex + i) % POOL_SIZE;
        if (!pool[idx].active) {
            nextSearchIndex = (idx + 1) % POOL_SIZE;
            return idx;
        }
    }

    // Pool is full — overwrite the oldest particle starting at the hint index.
    // This is the "graceful degradation" path: we never allocate, we just recycle.
    int idx = nextSearchIndex;
    nextSearchIndex = (nextSearchIndex + 1) % POOL_SIZE;
    return idx;
}

// ─── Emission ────────────────────────────────────────────────────────────────

void ParticleManager::Emit(Vector2 position, Vector2 velocity, Color color, float size, float lifeSpan) {
    int idx = GetNextAvailableIndex();
    Particle& p = pool[idx];

    p.position      = position;
    p.velocity      = velocity;
    p.color         = color;
    p.size          = size;
    p.lifeSpan      = lifeSpan;
    p.lifeRemaining = lifeSpan;
    p.active        = true;
    p.texture       = { 0 }; // No texture → draw as shape
    p.sourceRect    = { 0 };
    p.rotation      = 0.0f;
}

void ParticleManager::EmitSprite(Vector2 position, Vector2 velocity, Texture2D texture,
                                  Rectangle sourceRect, float rotation, float size,
                                  float lifeSpan, Color tint, bool silhouette) {
    int idx = GetNextAvailableIndex();
    Particle& p = pool[idx];

    p.position      = position;
    p.velocity      = velocity;
    p.color         = tint;
    p.size          = size;
    p.lifeSpan      = lifeSpan;
    p.lifeRemaining = lifeSpan;
    p.active        = true;
    p.texture       = texture;
    p.sourceRect    = sourceRect;
    p.rotation      = rotation;
    p.silhouette    = silhouette;
}

// ─── Update ──────────────────────────────────────────────────────────────────

void ParticleManager::Update(float deltaTime) {
    for (auto& p : pool) {
        if (!p.active) continue;

        p.lifeRemaining -= deltaTime;
        if (p.lifeRemaining <= 0.0f) {
            p.active = false;
            continue;
        }

        // Move based on velocity
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;

        // Fade alpha linearly as the particle dies
        float lifeRatio = p.lifeRemaining / p.lifeSpan; // 1.0 = fresh, 0.0 = dead
        p.color.a = (unsigned char)(lifeRatio * 255.0f);
    }
}

// ─── Draw ────────────────────────────────────────────────────────────────────

void ParticleManager::Draw() {
    for (const auto& p : pool) {
        if (!p.active) continue;

        if (p.texture.id != 0) {
            // Sprite particle — optionally use silhouette shader
            float halfSize = p.size * 0.5f;
            Rectangle dest   = { p.position.x, p.position.y, p.size, p.size };
            Vector2   origin = { halfSize, halfSize };

            if (p.silhouette && silhouetteShader.id != 0) {
                BeginShaderMode(silhouetteShader);
                DrawTexturePro(p.texture, p.sourceRect, dest, origin, p.rotation, p.color);
                EndShaderMode();
            } else {
                DrawTexturePro(p.texture, p.sourceRect, dest, origin, p.rotation, p.color);
            }
        } else {
            // Basic shape particle
            if (p.size <= 4.0f) {
                DrawCircleV(p.position, p.size, p.color);
            } else {
                int half = (int)(p.size * 0.5f);
                DrawRectangle(
                    (int)p.position.x - half,
                    (int)p.position.y - half,
                    (int)p.size,
                    (int)p.size,
                    p.color
                );
            }
        }
    }
}

// ─── Utility ─────────────────────────────────────────────────────────────────

void ParticleManager::Clear() {
    for (auto& p : pool) {
        p.active = false;
    }
}

// ─── Combat Emitters ─────────────────────────────────────────────────────────

void ParticleManager::SpawnDashTrail(Vector2 pos, Rectangle sourceRect, Texture2D texture,
                                      float rotation, bool flipX) {
    Rectangle rect = sourceRect;
    if (flipX) {
        rect.x    += rect.width;
        rect.width = -rect.width;
    }

    // silhouette=true: the shader reads alpha from the sprite for the shape
    // but outputs solid blue — no original colors bleed through.
    // Alpha 30 keeps the ghost subtle and stackable.
    Color blue = { 0, 80, 255, 30 };
    EmitSprite(pos, { 0.0f, 0.0f }, texture, rect, rotation,
               fabsf(sourceRect.width), 0.2f, blue, /*silhouette=*/true);
}

void ParticleManager::SpawnParrySparks(Vector2 pos, int count) {
    if (count <= 0) return;

    const float BASE_SPEED = 120.0f;
    const float LIFE       = 0.2f;

    for (int i = 0; i < count; ++i) {
        // Distribute sparks evenly around the full circle with a small random jitter
        float angle = ((float)i / (float)count) * 2.0f * PI
                    + ((float)(GetRandomValue(-30, 30)) / 100.0f); // ±0.3 rad jitter

        float speed = BASE_SPEED + (float)GetRandomValue(-30, 30);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };

        // All WHITE sparks — size 5px forces the DrawRectangle (square) branch
        float size = (float)GetRandomValue(5, 7);
        Emit(pos, vel, WHITE, size, LIFE);
    }
}

void ParticleManager::SpawnImpact(Vector2 pos, Vector2 projectileVelocity, Color color, int count) {
    if (count <= 0) return;

    // Reverse the projectile direction to get the "into-the-wall" bounce direction
    float len = Vector2Length(projectileVelocity);
    Vector2 baseDir = { -1.0f, 0.0f }; // Fallback if velocity is zero
    if (len > 0.001f) {
        baseDir = { -projectileVelocity.x / len, -projectileVelocity.y / len };
    }

    // Cone half-angle: ±45 degrees (PI/4 radians)
    const float HALF_CONE  = PI / 4.0f;
    const float BASE_SPEED = 90.0f;
    const float LIFE       = 0.25f;

    float baseAngle = atan2f(baseDir.y, baseDir.x);

    for (int i = 0; i < count; ++i) {
        float t     = (count > 1) ? ((float)i / (float)(count - 1)) : 0.5f;
        float angle = (baseAngle - HALF_CONE) + t * (2.0f * HALF_CONE)
                    + ((float)GetRandomValue(-10, 10) / 100.0f); // ±0.1 rad jitter

        float speed = BASE_SPEED + (float)GetRandomValue(-20, 40);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };

        // White squares: size 5-8px forces DrawRectangle branch; color param ignored, always WHITE
        float size = (float)GetRandomValue(5, 8);
        Color c    = WHITE;
        c.a        = (unsigned char)GetRandomValue(180, 255);

        Emit(pos, vel, c, size, LIFE);
    }
}

