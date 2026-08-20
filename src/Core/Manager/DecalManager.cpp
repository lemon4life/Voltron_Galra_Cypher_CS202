#include "Core/Manager/DecalManager.h"
#include "Core/Manager/LevelManager.h"
#include <cmath>

void DecalManager::AddCorpse(Vector2 pos, Texture2D tex, bool facingLeft, Vector2 slideVel) {
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
    corpses.clear();
}

void DecalManager::Update(
    float deltaTime,
    const LevelManager* levelManager
) {
    for (auto& corpse : corpses) {
        if (!corpse.settled) {
            // 1. Horizontal Sliding, Collision & Friction
            Rectangle corpseBox = { corpse.position.x - 8.0f, corpse.position.y - 8.0f, 16.0f, 16.0f };

            // X-axis collision
            corpse.position.x += corpse.slideVelocity.x * deltaTime;
            corpseBox.x = corpse.position.x - 8.0f;
            if (levelManager && levelManager->IsSolidCollision(corpseBox)) {
                corpse.position.x -= corpse.slideVelocity.x * deltaTime; // revert X
                corpse.slideVelocity.x *= -0.5f; // bounce and dampen
                corpseBox.x = corpse.position.x - 8.0f; // reset box X for Y check
            }

            // Y-axis collision
            corpse.position.y += corpse.slideVelocity.y * deltaTime;
            corpseBox.y = corpse.position.y - 8.0f;
            if (levelManager && levelManager->IsSolidCollision(corpseBox)) {
                corpse.position.y -= corpse.slideVelocity.y * deltaTime; // revert Y
                corpse.slideVelocity.y *= -0.5f; // bounce and dampen
            }
            
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
}

void DecalManager::Draw() {
    for (const auto& corpse : corpses) {
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
