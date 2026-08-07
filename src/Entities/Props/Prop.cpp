#include "Entities/Props/Prop.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Constants.h"
#include "raylib.h"
#include <algorithm>

namespace {
    constexpr int BOX_MAX_HEALTH = 1;
    constexpr float HEALTH_BAR_HEIGHT = 3.0f;
    constexpr float ANIM_FPS = 8.0f;
}

Prop::Prop(
    Vector2 tileCenter,
    GameObjectCell objectCell,
    IMapObjectDestroyAccess& destroyAccess,
    MapObjectId type
)
    : GameObject(tileCenter, GameObjectType::Box),
      destroyAccess(destroyAccess),
      objectCell(objectCell),
      propType(type),
      health(BOX_MAX_HEALTH),
      destructionQueued(false),
      animationTimer(0.0f),
      currentFrame(0) {
    boundingBox = GetBoundingBox();
}

void Prop::Update(float deltaTime) {
    if (propType == MapObjectId::Prop1) {
        animationTimer += deltaTime;
        if (animationTimer >= 1.0f / ANIM_FPS) {
            animationTimer = 0.0f;
            currentFrame = (currentFrame + 1) % 8;
        }
    }
}

void Prop::Draw() {
    // Left intentionally blank, rendering is handled by DrawBaseLayer and AddDepthRenderItems.
    // However, we can draw the health bar here since it should overlay everything.
    if (propType == MapObjectId::DestructibleBox && health < BOX_MAX_HEALTH) {
        Rectangle bounds = GetBoundingBox();
        float healthRatio = (float)health / (float)BOX_MAX_HEALTH;
        Rectangle barBackground = {
            bounds.x,
            bounds.y - bounds.height - HEALTH_BAR_HEIGHT - 2.0f,
            bounds.width,
            HEALTH_BAR_HEIGHT
        };
        Rectangle healthBar = barBackground;
        healthBar.width *= healthRatio;
        DrawRectangleRec(barBackground, DARKGRAY);
        DrawRectangleRec(healthBar, GREEN);
    }
}

void Prop::DrawBaseLayer() {
    Texture2D tex;
    int frames = 1;
    
    if (propType == MapObjectId::DestructibleBox) {
        tex = AssetManager::GetInstance().GetTexture("box");
    } else if (propType == MapObjectId::Prop1) {
        tex = AssetManager::GetInstance().GetTexture("prop1");
        if (tex.id == 0) {
            tex = AssetManager::GetInstance().LoadTexture2D("prop1", "assets/Objects/tall_object_1_8.png", true);
        }
        frames = 8;
    } else if (propType == MapObjectId::Prop2) {
        tex = AssetManager::GetInstance().GetTexture("prop2");
        if (tex.id == 0) {
            tex = AssetManager::GetInstance().LoadTexture2D("prop2", "assets/Objects/object_2.png", true);
        }
    } else if (propType == MapObjectId::MockWall) {
        tex = AssetManager::GetInstance().GetTexture("wallTileset");
        if (tex.id == 0) { // Fallback loading if not in AssetManager
            tex = AssetManager::GetInstance().LoadTexture2D("wallTileset", "assets/tileset/Galra_Walls.png", true);
        }
    }

    if (tex.id != 0) {
        float frameWidth = (float)tex.width / frames;
        float frameHeight = (float)tex.height;
        float splitY = frameHeight * 0.5f;

        Rectangle src = {currentFrame * frameWidth, splitY, frameWidth, frameHeight - splitY};
        
        Rectangle bounds = GetBoundingBox();
        float visualHeight = bounds.height / 0.75f;
        float destHeight = visualHeight * 0.5f;
        Rectangle dest = {bounds.x, bounds.y + bounds.height - destHeight, bounds.width, destHeight};

        DrawTexturePro(tex, src, dest, {0,0}, 0.0f, WHITE);
    }
}

void Prop::AddDepthRenderItems(std::vector<DepthRenderItem>& items) {
    items.push_back({
        GetBoundingBox().y + GetBoundingBox().height, // Base Y for sorting
        [this]() {
            Texture2D tex;
            int frames = 1;
            
            if (propType == MapObjectId::DestructibleBox) {
                tex = AssetManager::GetInstance().GetTexture("box");
            } else if (propType == MapObjectId::Prop1) {
                tex = AssetManager::GetInstance().GetTexture("prop1");
                if (tex.id == 0) {
                    tex = AssetManager::GetInstance().LoadTexture2D("prop1", "assets/Objects/tall_object_1_8.png", true);
                }
                frames = 8;
            } else if (propType == MapObjectId::Prop2) {
                tex = AssetManager::GetInstance().GetTexture("prop2");
                if (tex.id == 0) {
                    tex = AssetManager::GetInstance().LoadTexture2D("prop2", "assets/Objects/object_2.png", true);
                }
            } else if (propType == MapObjectId::MockWall) {
                tex = AssetManager::GetInstance().GetTexture("wallTileset");
            }

            if (tex.id != 0) {
                float frameWidth = (float)tex.width / frames;
                float frameHeight = (float)tex.height;
                float splitY = frameHeight * 0.5f;

                Rectangle src = {currentFrame * frameWidth, 0.0f, frameWidth, splitY};
                
                Rectangle bounds = GetBoundingBox();
                float visualHeight = bounds.height / 0.75f;
                float destHeight = visualHeight * 0.5f;
                // Top half is directly above the bottom half
                Rectangle dest = {bounds.x, bounds.y + bounds.height - visualHeight, bounds.width, destHeight};
                
                DrawTexturePro(tex, src, dest, {0,0}, 0.0f, WHITE);
            }
        }
    });
}

Rectangle Prop::GetBoundingBox() const {
    // Dynamically calculate bounding box from texture width (since base is width x width)
    Texture2D tex;
    int frames = 1;
    if (propType == MapObjectId::DestructibleBox) {
        tex = AssetManager::GetInstance().GetTexture("box");
    } else if (propType == MapObjectId::Prop1) {
        tex = AssetManager::GetInstance().GetTexture("prop1");
        if (tex.id == 0) {
            tex = AssetManager::GetInstance().LoadTexture2D("prop1", "assets/Objects/tall_object_1_8.png", true);
        }
        frames = 8;
    } else if (propType == MapObjectId::Prop2) {
        tex = AssetManager::GetInstance().GetTexture("prop2");
        if (tex.id == 0) {
            tex = AssetManager::GetInstance().LoadTexture2D("prop2", "assets/Objects/object_2.png", true);
        }
    } else if (propType == MapObjectId::MockWall) {
        tex = AssetManager::GetInstance().GetTexture("wallTileset");
    }

    float widthWorld = Constants::RENDER_TILE_SIZE; // Default fallback 32
    float heightWorld = Constants::RENDER_TILE_SIZE;
    if (tex.id != 0) {
        widthWorld = (float)tex.width / frames;
        heightWorld = (float)tex.height;
    }

    float collisionHeight = heightWorld * 0.75f;

    // Centered horizontally, bottom aligns with the Y axis 
    return {
        position.x - widthWorld / 2.0f,
        position.y - collisionHeight / 2.0f,
        widthWorld,
        collisionHeight
    };
}

void Prop::TakeDamage(int amount) {
    if (propType != MapObjectId::DestructibleBox) return; // Only destructible box takes damage

    if (amount <= 0 || destructionQueued) {
        return;
    }

    health = std::max(0, health - amount);
    if (health == 0) {
        destructionQueued = true;
        destroyAccess.QueueMapObjectDestruction(*this, objectCell);
    }
}
