#pragma once

#include "Core/Level/ILevelProvider.h"
#include "Core/LevelAccess.h"
#include "Core/Constants.h"
#include <vector>

class StaticLevelProvider : public ILevelProvider {
public:
    StaticLevelProvider(
        const std::vector<std::vector<int>>& mapGridLayer1,
        const std::vector<std::vector<int>>& mapGridLayer2,
        const std::vector<std::vector<MapObjectId>>& mapObjectGrid,
        Texture2D floorTileset,
        Texture2D wallTileset,
        int& gridRows,
        int& gridCols
    );

    void DrawBase() override;
    void GetDepthRenderItems(std::vector<DepthRenderItem>& items) override;
    bool IsSolidCollision(Rectangle box) const override;

private:
    const std::vector<std::vector<int>>& mapGridLayer1;
    const std::vector<std::vector<int>>& mapGridLayer2;
    const std::vector<std::vector<MapObjectId>>& mapObjectGrid;
    Texture2D floorTileset;
    Texture2D wallTileset;
    int& gridRows;
    int& gridCols;

    bool IsSolidMapObject(MapObjectId objectId) const;
    int pos_hash(int x, int y) const;
};
