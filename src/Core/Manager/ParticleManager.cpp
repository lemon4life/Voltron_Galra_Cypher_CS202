#include "Core/Manager/ParticleManager.h"
#include "raymath.h"
#include <cmath>

// ─── Constructor ────────────────────────────────────────────────────────────

ParticleManager::ParticleManager() : silhouetteShader({ 0 }) {
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

// ─── SpriteParticle Implementation ──────────────────────────────────────────

SpriteParticle::SpriteParticle(Vector2 pos, Vector2 vel, Color col, float sz, float life, 
                               Texture2D tex, Rectangle srcRect, float rot, bool sil)
    : position(pos), velocity(vel), color(col), size(sz), lifeSpan(life), lifeRemaining(life),
      texture(tex), sourceRect(srcRect), rotation(rot), silhouette(sil) {}

void SpriteParticle::Update(float deltaTime) {
    lifeRemaining -= deltaTime;
    
    // Move based on velocity
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;

    // Fade alpha linearly as the particle dies
    float lifeRatio = lifeRemaining / lifeSpan; // 1.0 = fresh, 0.0 = dead
    color.a = (unsigned char)(lifeRatio * 255.0f);
}

void SpriteParticle::Draw() const {
    if (texture.id != 0) {
        // Sprite particle — optionally use silhouette shader
        float halfSize = size * 0.5f;
        Rectangle dest   = { position.x, position.y, size, size };
        Vector2   origin = { halfSize, halfSize };

        if (silhouette) {
            Shader silhouetteShader = ParticleManager::GetInstance().GetSilhouetteShader();
            if (silhouetteShader.id != 0) {
                BeginShaderMode(silhouetteShader);
                DrawTexturePro(texture, sourceRect, dest, origin, rotation, color);
                EndShaderMode();
            } else {
                DrawTexturePro(texture, sourceRect, dest, origin, rotation, color);
            }
        } else {
            DrawTexturePro(texture, sourceRect, dest, origin, rotation, color);
        }
    } else {
        // Basic shape particle
        if (size <= 4.0f) {
            DrawCircleV(position, size, color);
        } else {
            int half = (int)(size * 0.5f);
            DrawRectangle(
                (int)position.x - half,
                (int)position.y - half,
                (int)size,
                (int)size,
                color
            );
        }
    }
}

bool SpriteParticle::IsDead() const {
    return lifeRemaining <= 0.0f;
}

// ─── Emission ────────────────────────────────────────────────────────────────

void ParticleManager::Emit(Vector2 position, Vector2 velocity, Color color, float size, float lifeSpan) {
    activeParticles.push_back(std::make_unique<SpriteParticle>(position, velocity, color, size, lifeSpan));
}

void ParticleManager::EmitSprite(Vector2 position, Vector2 velocity, Texture2D texture,
                                  Rectangle sourceRect, float rotation, float size,
                                  float lifeSpan, Color tint, bool silhouette) {
    activeParticles.push_back(std::make_unique<SpriteParticle>(position, velocity, tint, size, lifeSpan, texture, sourceRect, rotation, silhouette));
}

// ─── Update ──────────────────────────────────────────────────────────────────

void ParticleManager::Update(float deltaTime) {
    for (auto it = activeParticles.begin(); it != activeParticles.end(); ) {
        (*it)->Update(deltaTime);
        if ((*it)->IsDead()) {
            it = activeParticles.erase(it);
        } else {
            ++it;
        }
    }
}

// ─── Draw ────────────────────────────────────────────────────────────────────

void ParticleManager::Draw() {
    for (const auto& p : activeParticles) {
        p->Draw();
    }
}

// ─── Utility ─────────────────────────────────────────────────────────────────

void ParticleManager::Clear() {
    activeParticles.clear();
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

#include "Core/Visuals/DamageTextParticle.h"

void ParticleManager::SpawnDamageNumber(Vector2 pos, int damage) {
    // Generate a random slight upward/outward velocity (less extreme)
    float angle = (float)GetRandomValue(-120, -60) * DEG2RAD;
    float speed = (float)GetRandomValue(50, 100);
    Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
    
    // Create new DamageTextParticle and add it with a shorter lifetime
    activeParticles.push_back(std::make_unique<DamageTextParticle>(pos, vel, damage, 0.5f));
}

