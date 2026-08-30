#pragma once
#include "Core/World/ObjectId.h"
#include "raylib.h"

#include <cstddef>
#include <cmath>
#include <stdexcept>

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
    /// Creates a GameObject instance from the supplied configuration.
    GameObject(Vector2 pos, GameObjectType type)
        : objectId(nextObjectId++),
          objectType(type),
          position(pos),
          boundingBox{pos.x, pos.y, 0.0f, 0.0f} {
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y)) {
            throw std::invalid_argument(
                "Game object position must contain finite coordinates"
            );
        }
        ++liveCount;
    }
    /// Releases resources owned by this GameObject instance.
    virtual ~GameObject() { --liveCount; }

    /// Advances this component's state for the current frame.
    virtual void Update(float deltaTime) = 0;
    /// Renders this component using its current state and visual resources.
    virtual void Draw() = 0;

    /// Returns the current object type.
    GameObjectType GetObjectType() const noexcept { return objectType; }
    /// Returns the current object id.
    ObjectId GetObjectId() const noexcept { return objectId; }
    /// Returns the current live count.
    static std::size_t GetLiveCount() noexcept { return liveCount; }
    /// Returns the current position.
    Vector2 GetPosition() const { return position; }
    /// Updates the stored position.
    void SetPosition(Vector2 pos) {
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y)) {
            throw std::invalid_argument(
                "Game object position must contain finite coordinates"
            );
        }
        position = pos;
    }
    /// Returns the current bounding box.
    virtual Rectangle GetBoundingBox() const { return boundingBox; }
    /// Returns the current collision box.
    virtual Rectangle GetCollisionBox() const { return GetBoundingBox(); }
    /// Reports whether the solid navigation obstacle condition is satisfied.
    virtual bool IsSolidNavigationObstacle() const { return false; }
};
