#pragma once

#include "raylib.h"
#include <vector>
#include "Core/DepthRenderItem.h"

// Design Pattern - Bridge (also a runtime Strategy):
// Abstraction/context: LevelManager. Implementor interface: ILevelProvider.
// Concrete implementors: StaticLevelProvider and ProceduralLevelProvider.
// Rendering and collision vary without changing LevelManager's public API.
class ILevelProvider {
public:
    /// Releases resources owned by this ILevelProvider instance.
    virtual ~ILevelProvider() = default;

    /// Renders base.
    virtual void DrawBase() = 0;
    /// Renders portals on their fixed layer below actors.
    virtual void DrawPortalLayer() {}
    /// Returns the current depth render items.
    virtual void GetDepthRenderItems(std::vector<DepthRenderItem>& items) = 0;
    /// Reports whether the solid collision condition is satisfied.
    virtual bool IsSolidCollision(Rectangle box) const = 0;
    /// Appends static blocking colliders for tile.
    virtual void AppendStaticBlockingCollidersForTile(
        int tileX,
        int tileY,
        std::vector<Rectangle>& output
    ) const = 0;
};
