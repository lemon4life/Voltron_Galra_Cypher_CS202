#pragma once
#include "raylib.h"

enum class GameObjectType {
    Enemy,
    Box,
    Player,
    NPC,
    HubPaladinStand,
    Wall,
    Projectile,
    DoorGate,
    Prop
};

class GameObject {
private:
    const GameObjectType objectType;

protected:
    Vector2 position;
    Rectangle boundingBox;

public:
    GameObject(Vector2 pos, GameObjectType type)
        : objectType(type),
          position(pos),
          boundingBox{pos.x, pos.y, 0.0f, 0.0f} {}
    virtual ~GameObject() = default;

    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;

    GameObjectType GetObjectType() const noexcept { return objectType; }
    Vector2 GetPosition() const { return position; }
    void SetPosition(Vector2 pos) { position = pos; }
    virtual Rectangle GetBoundingBox() const { return boundingBox; }
    virtual Rectangle GetCollisionBox() const { return GetBoundingBox(); }
    virtual bool IsSolidNavigationObstacle() const { return false; }
};
