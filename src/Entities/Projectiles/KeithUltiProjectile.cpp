#include "Entities/Projectiles/KeithUltiProjectile.h"
#include "Core/Manager/GameManager.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "raymath.h"
#include <cstdlib>
#include <cmath>

/// Creates a KeithUltiProjectile instance from the supplied configuration.
KeithUltiProjectile::KeithUltiProjectile(
    Vector2 startPos,
    Vector2 dir,
    float speed,
    int directDamage,
    float flightTime,
    float lingeringTrailTime,
    float width,
    Texture2D waveTexture,
    Texture2D fireTexture
)
    : Projectile(
          startPos,
          Vector2Scale(dir, speed),
          flightTime + lingeringTrailTime,
          directDamage,
          waveTexture,
          false,
          width * 0.5f
      ),
      startPosition(startPos),
      travelDirection(dir),
      travelSpeed(speed),
      maxTravelTime(flightTime),
      trailDuration(lingeringTrailTime),
      maxTrailDuration(lingeringTrailTime),
      trailWidth(width),
      currentTrailLength(0.0f),
      maxTrailLength(speed * flightTime),
      lastSpawnDistance(0.0f),
      burnTickTimer(0.0f),
      fireAnimTexture(fireTexture)
{
    SetPiercing(true);
    SetReturning(false);
    SetMaxFlyTime(flightTime);
    float rot = atan2f(dir.y, dir.x) * RAD2DEG;
    SetFixedRotation(true, rot);
}

/// Spawns trail nodes.
void KeithUltiProjectile::SpawnTrailNodes(float fromDist, float toDist) {
    float step = 20.0f; // Spacing along the trail
    Vector2 normal = { -travelDirection.y, travelDirection.x };

    for (float d = fromDist; d <= toDist; d += step) {
        Vector2 centerOnPath = {
            startPosition.x + travelDirection.x * d,
            startPosition.y + travelDirection.y * d
        };

        // Spawn flame nodes scattered across the trail width
        for (int i = 0; i < 2; ++i) {
            float offset = ((float)rand() / (float)RAND_MAX - 0.5f) * (trailWidth * 0.7f);
            Vector2 nodePos = {
                centerOnPath.x + normal.x * offset,
                centerOnPath.y + normal.y * offset
            };

            FireTrailNode node;
            node.position = nodePos;
            node.currentFrame = rand() % 4;
            node.frameTimer = ((float)rand() / (float)RAND_MAX) * 0.1f;
            node.scale = 0.85f + ((float)rand() / (float)RAND_MAX) * 0.35f;
            fireNodes.push_back(node);
        }
    }
}

/// Updates trail.
void KeithUltiProjectile::UpdateTrail(float deltaTime) {
    // Cycle animation frames for all fire nodes
    for (auto& node : fireNodes) {
        node.frameTimer += deltaTime;
        if (node.frameTimer >= 0.08f) {
            node.frameTimer -= 0.08f;
            node.currentFrame = (node.currentFrame + 1) % 4;
        }
    }

    // Apply Burn status and DOT to enemies within the ground trail rectangle
    burnTickTimer += deltaTime;
    bool isTick = false;
    if (burnTickTimer >= 0.35f) {
        burnTickTimer = 0.0f;
        isTick = true;
    }

    const auto& enemies = GameManager::GetInstance().GetObjectManager().GetEnemies();
    for (Enemy* enemy : enemies) {
        if (!enemy || enemy->IsDead() || !enemy->IsEnabled()) continue;

        Vector2 ePos = enemy->GetPosition();
        Vector2 offset = Vector2Subtract(ePos, startPosition);

        // Project enemy offset onto travel direction and normal
        float dAlong = offset.x * travelDirection.x + offset.y * travelDirection.y;
        float dAcross = -offset.x * travelDirection.y + offset.y * travelDirection.x;

        if (dAlong >= 0.0f && dAlong <= currentTrailLength && fabsf(dAcross) <= (trailWidth * 0.5f + 16.0f)) {
            enemy->GetStatusComponent().AddEffect(EffectType::BURN, 3.0f, 15.0f);
            if (isTick) {
                enemy->TakeDamage(15);
                if (GetOwner()) {
                    Paladin* p = dynamic_cast<Paladin*>(GetOwner());
                    if (p) p->OnHitEnemy(15);
                }
            }
        }
    }
}

/// Advances this component's state for the current frame.
void KeithUltiProjectile::Update(float deltaTime) {
    if (!IsActive()) return;

    if (flightTimer < maxTravelTime) {
        // Wave is traveling
        Projectile::Update(deltaTime);
        currentTrailLength = Vector2Distance(startPosition, position);
        if (currentTrailLength > maxTrailLength) currentTrailLength = maxTrailLength;

        if (currentTrailLength > lastSpawnDistance + 20.0f) {
            SpawnTrailNodes(lastSpawnDistance, currentTrailLength);
            lastSpawnDistance = currentTrailLength;
        }
    } else {
        // Wave finished travel; lock velocity and count down lingering ground trail
        velocity = { 0.0f, 0.0f };
        trailDuration -= deltaTime;
        if (trailDuration <= 0.0f) {
            Destroy();
            return;
        }
    }

    UpdateTrail(deltaTime);
}

/// Renders this component using its current state and visual resources.
void KeithUltiProjectile::Draw() {
    if (!IsActive()) return;

    float alpha = 1.0f;
    if (trailDuration < 0.5f) {
        alpha = trailDuration / 0.5f;
        if (alpha < 0.0f) alpha = 0.0f;
    }

    // 1. Draw lingering low-opacity red ground zone
    if (currentTrailLength > 0.0f) {
        Rectangle trailRect = { startPosition.x, startPosition.y, currentTrailLength, trailWidth };
        Vector2 origin = { 0.0f, trailWidth * 0.5f };
        float rotDeg = GetRotationAngle();
        DrawRectanglePro(trailRect, origin, rotDeg, ColorAlpha(RED, 0.35f * alpha));
        DrawRectanglePro(trailRect, origin, rotDeg, ColorAlpha(ORANGE, 0.15f * alpha));
    }

    // 2. Draw animated 4-frame fire sprites along the trail
    if (fireAnimTexture.id != 0) {
        float frameWidth = (float)fireAnimTexture.width / 4.0f;
        float frameHeight = (float)fireAnimTexture.height;

        for (const auto& node : fireNodes) {
            Rectangle src = { (float)node.currentFrame * frameWidth, 0.0f, frameWidth, frameHeight };
            Rectangle dest = { node.position.x, node.position.y, frameWidth * node.scale, frameHeight * node.scale };
            Vector2 origin = { (frameWidth * node.scale) * 0.5f, (frameHeight * node.scale) * 0.5f };
            DrawTexturePro(fireAnimTexture, src, dest, origin, 0.0f, ColorAlpha(WHITE, alpha));
        }
    }

    // 3. Draw wave projectile while in flight
    if (flightTimer < maxTravelTime) {
        Projectile::Draw();
    }
}
