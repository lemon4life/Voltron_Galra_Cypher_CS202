#include "Entities/Items/DestructibleBox.h"
#include "Core/Manager/AssetManager.h"
#include "raylib.h"

#include <algorithm>

namespace {
    constexpr float BOX_SIZE = 32.0f;
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
        Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
        DrawTexturePro(tex, src, bounds, {0,0}, 0.0f, WHITE);
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
        position.x - BOX_SIZE / 2.0f,
        position.y - BOX_SIZE / 2.0f,
        BOX_SIZE,
        BOX_SIZE
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
