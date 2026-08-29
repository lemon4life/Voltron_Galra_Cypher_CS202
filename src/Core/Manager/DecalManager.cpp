#include "Core/Manager/DecalManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Level/VisibleWorld.h"
#include <cmath>
#include <algorithm>

namespace {
constexpr std::size_t MAX_CORPSE_DECALS = 128;
constexpr float CORPSE_DECAL_LIFETIME = 45.0f;
}

void DecalManager::AddCorpse(Vector2 pos, Texture2D tex, bool facingLeft, Vector2 slideVel) {
    if (corpses.size() >= MAX_CORPSE_DECALS) {
        corpses.erase(corpses.begin());
    }
    CorpseDecal corpse;
    corpse.position = pos;
    corpse.texture = tex;
    corpse.facingLeft = facingLeft;
    corpse.heightOffset = -5.0f;
    corpse.verticalVelocity = -80.0f;
    corpse.slideVelocity = slideVel;
    corpse.settled = false;
    corpses.push_back(corpse);
}

void DecalManager::Clear() {
    decltype(corpses){}.swap(corpses);
}

void DecalManager::Update(
    float deltaTime,
    const LevelManager* levelManager
) {
    for (auto& corpse : corpses) {
        corpse.age += deltaTime;
        if (!corpse.settled) {
            // 1. Horizontal Sliding, Collision & Friction
            Rectangle corpseBox = { corpse.position.x - 8.0f, corpse.position.y - 8.0f, 16.0f, 16.0f };

            Vector2 desiredDisplacement = {
                corpse.slideVelocity.x * deltaTime,
                corpse.slideVelocity.y * deltaTime
            };
            Vector2 appliedDisplacement = desiredDisplacement;
            if (levelManager) {
                CollisionMovementResult movement =
                    levelManager->ResolveSolidMovement(
                        corpseBox,
                        desiredDisplacement
                    );
                appliedDisplacement = movement.appliedDisplacement;
                if (movement.blockedX) corpse.slideVelocity.x *= -0.5f;
                if (movement.blockedY) corpse.slideVelocity.y *= -0.5f;
            }
            corpse.position.x += appliedDisplacement.x;
            corpse.position.y += appliedDisplacement.y;
            
            // High friction to make them skid to a halt quickly
            corpse.slideVelocity.x -= corpse.slideVelocity.x * 8.0f * deltaTime;
            corpse.slideVelocity.y -= corpse.slideVelocity.y * 8.0f * deltaTime;

            // 2. Gravity & Bouncing
            corpse.verticalVelocity += 800.0f * deltaTime; // Gravity
            corpse.heightOffset += corpse.verticalVelocity * deltaTime; 
            
            // Check for ground collision (heightOffset >= 0)
            if (corpse.heightOffset >= 0.0f) {
                corpse.heightOffset = 0.0f;
                corpse.verticalVelocity *= -0.15f; // Weaker bounce
                
                // Settle when both bouncing and sliding stop
                if (std::abs(corpse.verticalVelocity) < 30.0f && (std::abs(corpse.slideVelocity.x) < 10.0f && std::abs(corpse.slideVelocity.y) < 10.0f)) {
                    corpse.settled = true;
                }
            }
        }
    }
    corpses.erase(
        std::remove_if(
            corpses.begin(),
            corpses.end(),
            [](const CorpseDecal& corpse) {
                return corpse.age >= CORPSE_DECAL_LIFETIME;
            }
        ),
        corpses.end()
    );
}

void DecalManager::Draw() {
    for (const auto& corpse : corpses) {
        Rectangle corpseBounds = {
            corpse.position.x - corpse.texture.width * 0.5f,
            corpse.position.y - corpse.texture.height,
            static_cast<float>(corpse.texture.width),
            static_cast<float>(corpse.texture.height)
        };
        if (!IsWorldRectangleVisible(corpseBounds)) continue;
        Rectangle source = { 0, 0, (float)corpse.texture.width, (float)corpse.texture.height };
        if (corpse.facingLeft) {
            source.width = -source.width;
        }

        float drawY = corpse.position.y + corpse.heightOffset;
        Rectangle dest = { corpse.position.x, drawY, (float)corpse.texture.width, (float)corpse.texture.height };
        Vector2 origin = { (float)corpse.texture.width / 2.0f, (float)corpse.texture.height };

        Color darkTint = { 30, 30, 40, 255 }; // Almost black silhouette

        DrawTexturePro(corpse.texture, source, dest, origin, 0.0f, darkTint);
    }
}
