#pragma once

#include "Core/Level/ILevelProvider.h"
#include "Core/Constants.h"
#include <vector>

class StaticLevelProvider : public ILevelProvider {
public:
    StaticLevelProvider(
        const std::vector<std::vector<int>>& mapGridLayer1,
        const std::vector<std::vector<int>>& mapGridLayer2,
        Texture2D floorTileset,
        Texture2D wallTileset,
        int& gridRows,
        int& gridCols
    );

    void DrawBase() override;
    void GetDepthRenderItems(std::vector<DepthRenderItem>& items) override;
    bool IsSolidCollision(Rectangle box) const override;
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

    int pos_hash(int x, int y) const;
};
