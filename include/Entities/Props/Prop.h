#pragma once

#include "Core/LevelAccess.h"
#include "Entities/GameObject.h"
#include "Core/DepthRenderItem.h"

class Prop : public GameObject {
private:
    IMapObjectDestroyAccess& destroyAccess;
    const GameObjectCell objectCell;
    MapObjectId propType;
    int health;
    bool destructionQueued;
    float animationTimer;
    int currentFrame;

public:
    Prop(
        Vector2 tileCenter,
        GameObjectCell objectCell,
        IMapObjectDestroyAccess& destroyAccess,
        MapObjectId type
    );

    void Update(float deltaTime) override;
    void Draw() override; // May do nothing if rendering is handled externally
    Rectangle GetBoundingBox() const override;
    
    // Support for unified depth rendering
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items);
    
    bool IsSolidNavigationObstacle() const override { return propType == MapObjectId::DestructibleBox; }
    void DrawBaseLayer(); // Draws the bottom 16x16

    void TakeDamage(int amount);
    bool IsDestructionQueued() const { return destructionQueued; }
};
