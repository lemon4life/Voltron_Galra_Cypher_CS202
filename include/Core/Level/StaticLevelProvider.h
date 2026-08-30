#pragma once

#include "Core/Level/ILevelProvider.h"
#include "Core/Constants.h"
#include <vector>

class StaticLevelProvider : public ILevelProvider {
public:
    /// Creates a StaticLevelProvider instance from the supplied configuration.
    StaticLevelProvider(
        const std::vector<std::vector<int>>& mapGridLayer1,
        const std::vector<std::vector<int>>& mapGridLayer2,
        Texture2D floorTileset,
        Texture2D wallTileset,
        int& gridRows,
        int& gridCols
    );

    /// Renders base.
    void DrawBase() override;
    /// Returns the current depth render items.
    void GetDepthRenderItems(std::vector<DepthRenderItem>& items) override;
    /// Reports whether the solid collision condition is satisfied.
    bool IsSolidCollision(Rectangle box) const override;
    /// Appends static blocking colliders for tile.
    void AppendStaticBlockingCollidersForTile(
        int tileX,
        int tileY,
        std::vector<Rectangle>& output
    ) const override;

private:
    const std::vector<std::vector<int>>& mapGridLayer1;
    const std::vector<std::vector<int>>& mapGridLayer2;
    Texture2D floorTileset;
    Texture2D wallTileset;
    int& gridRows;
    int& gridCols;

    /// Implements the pos hash behavior for this component.
    int pos_hash(int x, int y) const;
};
