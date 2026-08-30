#pragma once

#include "Core/World/MapObject.h"

class Prop : public MapObject {
private:
    int health;
    bool destructionQueued;
    float animationTimer;
    int currentFrame;

public:
    /// Creates a Prop instance from the supplied configuration.
    Prop(
        Vector2 tileCenter,
        GameObjectCell objectCell,
        MapObjectId type
    );

    /// Returns the current destructible box sprite bounds.
    static Rectangle GetDestructibleBoxSpriteBounds(Vector2 tileCenter);
    /// Returns the current destructible box bounding box.
    static Rectangle GetDestructibleBoxBoundingBox(Vector2 tileCenter);
    /// Returns the current destructible box collision box.
    static Rectangle GetDestructibleBoxCollisionBox(Vector2 tileCenter);
    /// Returns the current map object sprite bounds.
    static Rectangle GetMapObjectSpriteBounds(
        Vector2 tileCenter,
        MapObjectId type
    );
    /// Returns the current map object bounding box.
    static Rectangle GetMapObjectBoundingBox(
        Vector2 tileCenter,
        MapObjectId type
    );
    /// Returns the current map object collision box.
    static Rectangle GetMapObjectCollisionBox(
        Vector2 tileCenter,
        MapObjectId type
    );

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Returns the current bounding box.
    Rectangle GetBoundingBox() const override;
    /// Returns the current collision box.
    Rectangle GetCollisionBox() const override;
    
    // Support for unified depth rendering
    /// Adds depth render items.
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items) override;
    
    /// Reports whether the solid condition is satisfied.
    bool IsSolid() const override;
    /// Renders base layer.
    void DrawBaseLayer() override; // Draws the bottom 16x16

    /// Applies incoming damage after this object handles defenses and state-specific rules.
    void TakeDamage(int amount) override;
    /// Reports whether the destroyed condition is satisfied.
    bool IsDestroyed() const override { return destructionQueued; }
};
