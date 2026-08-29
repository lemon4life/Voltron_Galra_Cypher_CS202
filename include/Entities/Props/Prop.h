#pragma once

#include "Core/World/MapObject.h"

class Prop : public MapObject {
private:
    int health;
    bool destructionQueued;
    float animationTimer;
    int currentFrame;

public:
    Prop(
        Vector2 tileCenter,
        GameObjectCell objectCell,
        MapObjectId type
    );

    static Rectangle GetDestructibleBoxSpriteBounds(Vector2 tileCenter);
    static Rectangle GetDestructibleBoxBoundingBox(Vector2 tileCenter);
    static Rectangle GetDestructibleBoxCollisionBox(Vector2 tileCenter);
    static Rectangle GetMapObjectSpriteBounds(
        Vector2 tileCenter,
        MapObjectId type
    );
    static Rectangle GetMapObjectBoundingBox(
        Vector2 tileCenter,
        MapObjectId type
    );
    static Rectangle GetMapObjectCollisionBox(
        Vector2 tileCenter,
        MapObjectId type
    );

    void Update(float deltaTime) override;
    Rectangle GetBoundingBox() const override;
    Rectangle GetCollisionBox() const override;
    
    // Support for unified depth rendering
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items) override;
    
    bool IsSolid() const override;
    void DrawBaseLayer() override; // Draws the bottom 16x16

    void TakeDamage(int amount) override;
    bool IsDestroyed() const override { return destructionQueued; }
};
