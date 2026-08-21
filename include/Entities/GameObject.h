#pragma once
#include "Core/World/ObjectId.h"
#include "raylib.h"

#include <cstddef>

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
    inline static ObjectId nextObjectId = 1;
    inline static std::size_t liveCount = 0;
    const ObjectId objectId;
    const GameObjectType objectType;

protected:
    Vector2 position;
    Rectangle boundingBox;

public:
    GameObject(Vector2 pos, GameObjectType type)
        : objectId(nextObjectId++),
          objectType(type),
          position(pos),
          boundingBox{pos.x, pos.y, 0.0f, 0.0f} {
        ++liveCount;
    }
    virtual ~GameObject() { --liveCount; }

    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;

    GameObjectType GetObjectType() const noexcept { return objectType; }
    ObjectId GetObjectId() const noexcept { return objectId; }
    static std::size_t GetLiveCount() noexcept { return liveCount; }
    Vector2 GetPosition() const { return position; }
    void SetPosition(Vector2 pos) { position = pos; }
    virtual Rectangle GetBoundingBox() const { return boundingBox; }
    virtual Rectangle GetCollisionBox() const { return GetBoundingBox(); }
    virtual bool IsSolidNavigationObstacle() const { return false; }
};
