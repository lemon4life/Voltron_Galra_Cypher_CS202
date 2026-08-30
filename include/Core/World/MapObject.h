#pragma once

#include "Core/DepthRenderItem.h"
#include "Core/LevelAccess.h"
#include "Core/World/ObjectId.h"
#include "raylib.h"

#include <cstdint>
#include <cstddef>
#include <vector>

class MapObject {
private:
    inline static MapObjectHandle nextHandle = 1;
    inline static std::size_t liveCount = 0;
    const MapObjectHandle handle;

protected:
    Vector2 position;
    Rectangle boundingBox;
    GameObjectCell objectCell;
    MapObjectId mapObjectType;

public:
    /// Creates a MapObject instance from the supplied configuration.
    MapObject(
        Vector2 position,
        Rectangle bounds,
        GameObjectCell cell,
        MapObjectId type
    )
        : handle(nextHandle++),
          position(position),
          boundingBox(bounds),
          objectCell(cell),
          mapObjectType(type) {
        ++liveCount;
    }

    /// Releases resources owned by this MapObject instance.
    virtual ~MapObject() { --liveCount; }

    /// Returns the current handle.
    MapObjectHandle GetHandle() const noexcept { return handle; }
    /// Returns the current live count.
    static std::size_t GetLiveCount() noexcept { return liveCount; }
    /// Returns the current position.
    Vector2 GetPosition() const noexcept { return position; }
    /// Returns the current object cell.
    GameObjectCell GetObjectCell() const noexcept { return objectCell; }
    /// Returns the current map object type.
    MapObjectId GetMapObjectType() const noexcept { return mapObjectType; }

    /// Advances this component's state for the current frame.
    virtual void Update(float deltaTime) = 0;
    /// Returns the current bounding box.
    virtual Rectangle GetBoundingBox() const { return boundingBox; }
    /// Returns the current collision box.
    virtual Rectangle GetCollisionBox() const { return GetBoundingBox(); }
    /// Reports whether the solid condition is satisfied.
    virtual bool IsSolid() const = 0;
    /// Reports whether the destroyed condition is satisfied.
    virtual bool IsDestroyed() const { return false; }
    /// Applies incoming damage after this object handles defenses and state-specific rules.
    virtual void TakeDamage(int amount) { (void)amount; }

    /// Renders base layer.
    virtual void DrawBaseLayer() = 0;
    /// Adds depth render items.
    virtual void AddDepthRenderItems(
        std::vector<DepthRenderItem>& items
    ) = 0;
};
