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

    /// Returns the current box world scale.
    float GetBoxWorldScale() {
        return Constants::RENDER_TILE_SIZE / BOX_SPRITE_WIDTH;
    }

    /// Returns the current box sprite top left.
    Vector2 GetBoxSpriteTopLeft(Vector2 tileCenter) {
        float scale = GetBoxWorldScale();
        return {
            tileCenter.x - BOX_SPRITE_WIDTH * scale * 0.5f,
            tileCenter.y + Constants::RENDER_TILE_SIZE * 0.5f -
                BOX_BODY_BOTTOM * scale
        };
    }

    /// Returns the current map object frame size.
    void GetMapObjectFrameSize(
        MapObjectId type,
        float& width,
        float& height
    ) {
        width = Constants::RENDER_TILE_SIZE;
        height = Constants::RENDER_TILE_SIZE;

        Texture2D texture = {};
        int frames = 1;
        AssetManager& assets = AssetManager::GetInstance();
        if (type == MapObjectId::Prop1) {
            texture = assets.GetTexture("prop1");
            frames = 8;
        } else if (type == MapObjectId::Prop2) {
            texture = assets.GetTexture("prop2");
        } else if (type == MapObjectId::MockWall) {
            texture = assets.GetTexture("wallTileset");
        }

        if (texture.id != 0) {
            width = static_cast<float>(texture.width) / frames;
            height = static_cast<float>(texture.height);
        }
    }
}

/// Creates a Prop instance from the supplied configuration.
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

/// Advances this component's state for the current frame.
void Prop::Update(float deltaTime) {
    if (mapObjectType == MapObjectId::Prop1) {
        animationTimer += deltaTime;
        if (animationTimer >= 1.0f / ANIM_FPS) {
            animationTimer = 0.0f;
            currentFrame = (currentFrame + 1) % 8;
        }
    }
}

/// Renders base layer.
void Prop::DrawBaseLayer() {
    if (mapObjectType == MapObjectId::DestructibleBox) {
        Texture2D tex = AssetManager::GetInstance().GetTexture("box");
        if (tex.id == 0) return;

        float scale = GetBoxWorldScale();
        Rectangle spriteBounds = GetDestructibleBoxSpriteBounds(position);
        Rectangle source = {
            0.0f,
            BOX_SHADOW_TOP,
            BOX_SPRITE_WIDTH,
            BOX_SHADOW_HEIGHT
        };
        Rectangle destination = {
            spriteBounds.x,
            spriteBounds.y + BOX_SHADOW_TOP * scale,
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
        frames = 8;
    } else if (mapObjectType == MapObjectId::Prop2) {
        tex = AssetManager::GetInstance().GetTexture("prop2");
    } else if (mapObjectType == MapObjectId::MockWall) {
        tex = AssetManager::GetInstance().GetTexture("wallTileset");
    }

    if (tex.id != 0) {
        float frameWidth = (float)tex.width / frames;
        float frameHeight = (float)tex.height;
        float splitY = frameHeight * 0.5f;
        Rectangle src = {currentFrame * frameWidth, splitY, frameWidth, frameHeight - splitY};
        Rectangle spriteBounds = GetMapObjectSpriteBounds(
            position,
            mapObjectType
        );
        Rectangle dest = {
            spriteBounds.x,
            spriteBounds.y + spriteBounds.height * 0.5f,
            spriteBounds.width,
            spriteBounds.height * 0.5f
        };
        DrawTexturePro(tex, src, dest, {0,0}, 0.0f, WHITE);
    }
}

/// Adds depth render items.
void Prop::AddDepthRenderItems(std::vector<DepthRenderItem>& items) {
    items.push_back({
        GetBoundingBox().y + GetBoundingBox().height, // Base Y for sorting
        [this]() {
            if (mapObjectType == MapObjectId::DestructibleBox) {
                Texture2D boxTexture =
                    AssetManager::GetInstance().GetTexture("box");
                if (boxTexture.id == 0) return;

                float scale = GetBoxWorldScale();
                Rectangle spriteBounds =
                    GetDestructibleBoxSpriteBounds(position);
                Rectangle source = {
                    0.0f,
                    0.0f,
                    BOX_SPRITE_WIDTH,
                    BOX_BODY_BOTTOM
                };
                Rectangle destination = {
                    spriteBounds.x,
                    spriteBounds.y,
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
                tex = AssetManager::GetInstance().GetTexture(
                    "prop1"
                );
                frames = 8;
            } else if (mapObjectType == MapObjectId::Prop2) {
                tex = AssetManager::GetInstance().GetTexture("prop2");
            } else if (mapObjectType == MapObjectId::MockWall) {
                tex = AssetManager::GetInstance().GetTexture("wallTileset");
            }

            if (tex.id != 0) {
                float frameWidth = (float)tex.width / frames;
                float frameHeight = (float)tex.height;
                float splitY = frameHeight * 0.5f;

                Rectangle src = {currentFrame * frameWidth, 0.0f, frameWidth, splitY};
                Rectangle spriteBounds = GetMapObjectSpriteBounds(
                    position,
                    mapObjectType
                );
                Rectangle dest = {
                    spriteBounds.x,
                    spriteBounds.y,
                    spriteBounds.width,
                    spriteBounds.height * 0.5f
                };
                
                DrawTexturePro(tex, src, dest, {0,0}, 0.0f, WHITE);
            }
        }
    });
}

/// Returns the current bounding box.
Rectangle Prop::GetBoundingBox() const {
    return GetMapObjectBoundingBox(position, mapObjectType);
}

/// Returns the current collision box.
Rectangle Prop::GetCollisionBox() const {
    return GetMapObjectCollisionBox(position, mapObjectType);
}

/// Applies incoming damage after this object handles defenses and state-specific rules.
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

/// Returns the current destructible box sprite bounds.
Rectangle Prop::GetDestructibleBoxSpriteBounds(Vector2 tileCenter) {
    float scale = GetBoxWorldScale();
    Vector2 topLeft = GetBoxSpriteTopLeft(tileCenter);
    return {
        topLeft.x,
        topLeft.y,
        BOX_SPRITE_WIDTH * scale,
        (BOX_BODY_BOTTOM + BOX_SHADOW_HEIGHT) * scale
    };
}

/// Returns the current destructible box bounding box.
Rectangle Prop::GetDestructibleBoxBoundingBox(Vector2 tileCenter) {
    float scale = GetBoxWorldScale();
    Rectangle spriteBounds = GetDestructibleBoxSpriteBounds(tileCenter);
    return {
        spriteBounds.x + BOX_HITBOX_X * scale,
        spriteBounds.y + BOX_HITBOX_Y * scale,
        BOX_HITBOX_WIDTH * scale,
        BOX_HITBOX_HEIGHT * scale
    };
}

/// Returns the current destructible box collision box.
Rectangle Prop::GetDestructibleBoxCollisionBox(Vector2 tileCenter) {
    float scale = GetBoxWorldScale();
    Rectangle spriteBounds = GetDestructibleBoxSpriteBounds(tileCenter);
    return {
        spriteBounds.x,
        spriteBounds.y + BOX_COLLISION_Y * scale,
        BOX_COLLISION_WIDTH * scale,
        BOX_COLLISION_HEIGHT * scale
    };
}

/// Returns the current map object sprite bounds.
Rectangle Prop::GetMapObjectSpriteBounds(
    Vector2 tileCenter,
    MapObjectId type
) {
    if (type == MapObjectId::DestructibleBox) {
        return GetDestructibleBoxSpriteBounds(tileCenter);
    }

    float width = Constants::RENDER_TILE_SIZE;
    float height = Constants::RENDER_TILE_SIZE;
    GetMapObjectFrameSize(type, width, height);

    float bottom = tileCenter.y + height * 0.375f;
    if (type == MapObjectId::Prop2) {
        // Object's last sprite row sits on the bottom row of its host tile.
        bottom = tileCenter.y + Constants::RENDER_TILE_SIZE * 0.5f;
    }
    return {
        tileCenter.x - width * 0.5f,
        bottom - height,
        width,
        height
    };
}

/// Returns the current map object bounding box.
Rectangle Prop::GetMapObjectBoundingBox(
    Vector2 tileCenter,
    MapObjectId type
) {
    if (type == MapObjectId::DestructibleBox) {
        return GetDestructibleBoxBoundingBox(tileCenter);
    }

    Rectangle spriteBounds = GetMapObjectSpriteBounds(tileCenter, type);
    float collisionHeight = spriteBounds.height * 0.75f;
    if (type == MapObjectId::Prop1) {
        // Tall Object uses only the lowest two tiles for damage/collision.
        collisionHeight = Constants::RENDER_TILE_SIZE * 2.0f;
    }
    return {
        spriteBounds.x,
        spriteBounds.y + spriteBounds.height - collisionHeight,
        spriteBounds.width,
        collisionHeight
    };
}

/// Returns the current map object collision box.
Rectangle Prop::GetMapObjectCollisionBox(
    Vector2 tileCenter,
    MapObjectId type
) {
    if (type == MapObjectId::DestructibleBox) {
        return GetDestructibleBoxCollisionBox(tileCenter);
    }
    return GetMapObjectBoundingBox(tileCenter, type);
}

/// Reports whether the solid condition is satisfied.
bool Prop::IsSolid() const {
    return mapObjectType == MapObjectId::DestructibleBox ||
        mapObjectType == MapObjectId::Prop1 ||
        mapObjectType == MapObjectId::Prop2 ||
        mapObjectType == MapObjectId::MockWall;
}
