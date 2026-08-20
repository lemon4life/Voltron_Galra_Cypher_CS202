#pragma once

#include "raylib.h"
#include <vector>
#include "Core/DepthRenderItem.h"

class ILevelProvider {
public:
    virtual ~ILevelProvider() = default;

    virtual void DrawBase() = 0;
    virtual void GetDepthRenderItems(std::vector<DepthRenderItem>& items) = 0;
    virtual bool IsSolidCollision(Rectangle box) const = 0;
    virtual void AppendStaticBlockingCollidersForTile(
        int tileX,
        int tileY,
        std::vector<Rectangle>& output
    ) const = 0;
};
