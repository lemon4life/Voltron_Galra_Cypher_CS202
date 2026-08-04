#include "Entities/Items/DestructibleBox.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Constants.h"
#include "raylib.h"

#include <algorithm>

namespace {
    constexpr int BOX_MAX_HEALTH = 100;
    constexpr float HEALTH_BAR_HEIGHT = 3.0f;
    constexpr Color BOX_COLOR = { 139, 90, 43, 255 };
    constexpr Color BOX_EDGE_COLOR = { 73, 42, 20, 255 };
}

DestructibleBox::DestructibleBox(
    Vector2 tileCenter,
    GameObjectCell objectCell,
    IMapObjectDestroyAccess& destroyAccess
)
    : GameObject(tileCenter, GameObjectType::Box),
      destroyAccess(destroyAccess),
      objectCell(objectCell),
      health(BOX_MAX_HEALTH),
      destructionQueued(false) {
    boundingBox = GetBoundingBox();
}

void DestructibleBox::Update(float deltaTime) {
    (void)deltaTime;
}

void DestructibleBox::Draw() {
    Rectangle bounds = GetBoundingBox();
    Texture2D tex = AssetManager::GetInstance().GetTexture("box");
    if (tex.id != 0) {
        Rectangle boxTopSrc = {0, 0, 16, 16};
        Rectangle boxBottomSrc = {0, 16, 16, 16};
        
        // Draw bottom first for depth, or maybe top first? Wait, top is higher up.
        // It's a single 16x32 texture? The prompt says: "Rectangle boxTopSrc = {0, 0, 16, 16}; Rectangle boxBottomSrc = {0, 16, 16, 16};"
        // Let's just draw the top and bottom on top of each other or offset? 
        // A standard destructible box on a 2D map might just occupy one tile in terms of collision, but the graphic is 2 tiles high (16x32).
        // Let's draw the top part shifted up by one tile height, and bottom part at the bounds.
        // Wait, if bounds is RENDER_TILE_SIZE (which is 48x48), drawing two 48x48 tiles means the object appears 96 pixels tall.
        
        Rectangle topBounds = {bounds.x, bounds.y - bounds.height, bounds.width, bounds.height};
        Rectangle bottomBounds = bounds;
        
        DrawTexturePro(tex, boxTopSrc, topBounds, {0,0}, 0.0f, WHITE);
        DrawTexturePro(tex, boxBottomSrc, bottomBounds, {0,0}, 0.0f, WHITE);
    } else {
        DrawRectangleRec(bounds, BOX_COLOR);
        DrawRectangleLinesEx(bounds, 2.0f, BOX_EDGE_COLOR);
    }

    if (health < BOX_MAX_HEALTH) {
        float healthRatio = (float)health / (float)BOX_MAX_HEALTH;
        Rectangle barBackground = {
            bounds.x,
            bounds.y - HEALTH_BAR_HEIGHT - 2.0f,
            bounds.width,
            HEALTH_BAR_HEIGHT
        };
        Rectangle healthBar = barBackground;
        healthBar.width *= healthRatio;
        DrawRectangleRec(barBackground, DARKGRAY);
        DrawRectangleRec(healthBar, GREEN);
    }
}

Rectangle DestructibleBox::GetBoundingBox() const {
    return {
        position.x - Constants::RENDER_TILE_SIZE / 2.0f,
        position.y - Constants::RENDER_TILE_SIZE / 2.0f,
        Constants::RENDER_TILE_SIZE,
        Constants::RENDER_TILE_SIZE
    };
}

void DestructibleBox::TakeDamage(int amount) {
    if (amount <= 0 || destructionQueued) {
        return;
    }

    health = std::max(0, health - amount);
    if (health == 0) {
        destructionQueued = true;
        destroyAccess.QueueMapObjectDestruction(*this, objectCell);
    }
}
