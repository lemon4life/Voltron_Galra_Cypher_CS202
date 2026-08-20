#include "Entities/Props/Prop.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Constants.h"
#include "raylib.h"
#include <algorithm>

namespace {
    constexpr int BOX_MAX_HEALTH = 1;
    constexpr float ANIM_FPS = 8.0f;
}

Prop::Prop(
    Vector2 tileCenter,
    GameObjectCell objectCell,
    MapObjectId type
)
    : MapObject(
          tileCenter,
          { tileCenter.x, tileCenter.y, 0.0f, 0.0f },
          objectCell,
          type
      ),
      health(BOX_MAX_HEALTH),
      destructionQueued(false),
      animationTimer(0.0f),
      currentFrame(0) {
    boundingBox = GetBoundingBox();
}

void Prop::Update(float deltaTime) {
    if (mapObjectType == MapObjectId::Prop1) {
        animationTimer += deltaTime;
        if (animationTimer >= 1.0f / ANIM_FPS) {
            animationTimer = 0.0f;
            currentFrame = (currentFrame + 1) % 8;
        }
    }
}

void Prop::DrawBaseLayer() {
    Texture2D tex;
    int frames = 1;
    
    if (mapObjectType == MapObjectId::DestructibleBox) {
        tex = AssetManager::GetInstance().GetTexture("box");
    } else if (mapObjectType == MapObjectId::Prop1) {
        tex = AssetManager::GetInstance().GetTexture("prop1");
        if (tex.id == 0) {
            tex = AssetManager::GetInstance().LoadTexture2D("prop1", "assets/Objects/tall_object_1_8.png", true);
        }
        frames = 8;
    } else if (mapObjectType == MapObjectId::Prop2) {
        tex = AssetManager::GetInstance().GetTexture("prop2");
        if (tex.id == 0) {
            tex = AssetManager::GetInstance().LoadTexture2D("prop2", "assets/Objects/object_2.png", true);
        }
    } else if (mapObjectType == MapObjectId::MockWall) {
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
            
            if (mapObjectType == MapObjectId::DestructibleBox) {
                tex = AssetManager::GetInstance().GetTexture("box");
            } else if (mapObjectType == MapObjectId::Prop1) {
                tex = AssetManager::GetInstance().GetTexture("prop1");
                if (tex.id == 0) {
                    tex = AssetManager::GetInstance().LoadTexture2D("prop1", "assets/Objects/tall_object_1_8.png", true);
                }
                frames = 8;
            } else if (mapObjectType == MapObjectId::Prop2) {
                tex = AssetManager::GetInstance().GetTexture("prop2");
                if (tex.id == 0) {
                    tex = AssetManager::GetInstance().LoadTexture2D("prop2", "assets/Objects/object_2.png", true);
                }
            } else if (mapObjectType == MapObjectId::MockWall) {
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
    if (mapObjectType == MapObjectId::DestructibleBox) {
        tex = AssetManager::GetInstance().GetTexture("box");
    } else if (mapObjectType == MapObjectId::Prop1) {
        tex = AssetManager::GetInstance().GetTexture("prop1");
        if (tex.id == 0) {
            tex = AssetManager::GetInstance().LoadTexture2D("prop1", "assets/Objects/tall_object_1_8.png", true);
        }
        frames = 8;
    } else if (mapObjectType == MapObjectId::Prop2) {
        tex = AssetManager::GetInstance().GetTexture("prop2");
        if (tex.id == 0) {
            tex = AssetManager::GetInstance().LoadTexture2D("prop2", "assets/Objects/object_2.png", true);
        }
    } else if (mapObjectType == MapObjectId::MockWall) {
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
    if (mapObjectType != MapObjectId::DestructibleBox) return;

    if (amount <= 0 || destructionQueued) {
        return;
    }

    health = std::max(0, health - amount);
    if (health == 0) {
        destructionQueued = true;
        AudioManager::GetInstance().PlaySoundEffect("fx_box_destroy");
    }
}

bool Prop::IsSolid() const {
    return mapObjectType == MapObjectId::DestructibleBox ||
        mapObjectType == MapObjectId::Prop1 ||
        mapObjectType == MapObjectId::Prop2 ||
        mapObjectType == MapObjectId::MockWall;
}
