#include "Entities/Props/Prop.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Constants.h"
#include "raylib.h"
#include <algorithm>

namespace {
    constexpr int BOX_MAX_HEALTH = 1;
    constexpr float ANIM_FPS = 8.0f;
    constexpr float BOX_SPRITE_WIDTH = 16.0f;
    constexpr float BOX_BODY_BOTTOM = 24.0f;
    constexpr float BOX_SHADOW_TOP = 24.0f;
    constexpr float BOX_SHADOW_HEIGHT = 8.0f;
    constexpr float BOX_HITBOX_X = 1.0f;
    constexpr float BOX_HITBOX_Y = 9.0f;
    constexpr float BOX_HITBOX_WIDTH = 14.0f;
    constexpr float BOX_HITBOX_HEIGHT = 14.0f;
    constexpr float BOX_COLLISION_Y = 8.0f;
    constexpr float BOX_COLLISION_WIDTH = 16.0f;
    constexpr float BOX_COLLISION_HEIGHT = 16.0f;

    float GetBoxWorldScale() {
        return Constants::RENDER_TILE_SIZE / BOX_SPRITE_WIDTH;
    }

    Vector2 GetBoxSpriteTopLeft(Vector2 tileCenter) {
        float scale = GetBoxWorldScale();
        return {
            tileCenter.x - BOX_SPRITE_WIDTH * scale * 0.5f,
            tileCenter.y + Constants::RENDER_TILE_SIZE * 0.5f -
                BOX_BODY_BOTTOM * scale
        };
    }
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
    if (mapObjectType == MapObjectId::DestructibleBox) {
        Texture2D tex = AssetManager::GetInstance().GetTexture("box");
        if (tex.id == 0) return;

        float scale = GetBoxWorldScale();
        Vector2 topLeft = GetBoxSpriteTopLeft(position);
        Rectangle source = {
            0.0f,
            BOX_SHADOW_TOP,
            BOX_SPRITE_WIDTH,
            BOX_SHADOW_HEIGHT
        };
        Rectangle destination = {
            topLeft.x,
            topLeft.y + BOX_SHADOW_TOP * scale,
            BOX_SPRITE_WIDTH * scale,
            BOX_SHADOW_HEIGHT * scale
        };
        DrawTexturePro(
            tex,
            source,
            destination,
            { 0.0f, 0.0f },
            0.0f,
            WHITE
        );
        return;
    }

    Texture2D tex;
    int frames = 1;
    
    if (mapObjectType == MapObjectId::Prop1) {
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
            if (mapObjectType == MapObjectId::DestructibleBox) {
                Texture2D boxTexture =
                    AssetManager::GetInstance().GetTexture("box");
                if (boxTexture.id == 0) return;

                float scale = GetBoxWorldScale();
                Vector2 topLeft = GetBoxSpriteTopLeft(position);
                Rectangle source = {
                    0.0f,
                    0.0f,
                    BOX_SPRITE_WIDTH,
                    BOX_BODY_BOTTOM
                };
                Rectangle destination = {
                    topLeft.x,
                    topLeft.y,
                    BOX_SPRITE_WIDTH * scale,
                    BOX_BODY_BOTTOM * scale
                };
                DrawTexturePro(
                    boxTexture,
                    source,
                    destination,
                    { 0.0f, 0.0f },
                    0.0f,
                    WHITE
                );
                return;
            }

            Texture2D tex;
            int frames = 1;
            
            if (mapObjectType == MapObjectId::Prop1) {
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
    if (mapObjectType == MapObjectId::DestructibleBox) {
        float scale = GetBoxWorldScale();
        Vector2 topLeft = GetBoxSpriteTopLeft(position);
        return {
            topLeft.x + BOX_HITBOX_X * scale,
            topLeft.y + BOX_HITBOX_Y * scale,
            BOX_HITBOX_WIDTH * scale,
            BOX_HITBOX_HEIGHT * scale
        };
    }

    // Dynamically calculate bounding box from texture width (since base is width x width)
    Texture2D tex;
    int frames = 1;
    if (mapObjectType == MapObjectId::Prop1) {
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

Rectangle Prop::GetCollisionBox() const {
    if (mapObjectType != MapObjectId::DestructibleBox) {
        return GetBoundingBox();
    }

    float scale = GetBoxWorldScale();
    Vector2 topLeft = GetBoxSpriteTopLeft(position);
    return {
        topLeft.x,
        topLeft.y + BOX_COLLISION_Y * scale,
        BOX_COLLISION_WIDTH * scale,
        BOX_COLLISION_HEIGHT * scale
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
