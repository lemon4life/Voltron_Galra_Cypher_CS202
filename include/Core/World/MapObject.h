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

    virtual ~MapObject() { --liveCount; }

    MapObjectHandle GetHandle() const noexcept { return handle; }
    static std::size_t GetLiveCount() noexcept { return liveCount; }
    Vector2 GetPosition() const noexcept { return position; }
    GameObjectCell GetObjectCell() const noexcept { return objectCell; }
    MapObjectId GetMapObjectType() const noexcept { return mapObjectType; }

    virtual void Update(float deltaTime) = 0;
    virtual Rectangle GetBoundingBox() const { return boundingBox; }
    virtual Rectangle GetCollisionBox() const { return GetBoundingBox(); }
    virtual bool IsSolid() const = 0;
    virtual bool IsDestroyed() const { return false; }
    virtual void TakeDamage(int amount) { (void)amount; }

    virtual void DrawBaseLayer() = 0;
    virtual void AddDepthRenderItems(
        std::vector<DepthRenderItem>& items
    ) = 0;
};
