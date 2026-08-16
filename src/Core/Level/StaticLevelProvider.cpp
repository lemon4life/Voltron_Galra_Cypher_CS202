#include "Core/Level/StaticLevelProvider.h"
#include <cmath>
#include <algorithm>

namespace {
    constexpr float COLLISION_EDGE_PADDING = 0.001f;
    constexpr float PARTIAL_WALL_WIDTH = 9.0f;
    constexpr float RIGHT_PARTIAL_WALL_OFFSET = Constants::RENDER_TILE_SIZE - PARTIAL_WALL_WIDTH;
    constexpr int DRAW_PADDING_TILES = 20;
}

StaticLevelProvider::StaticLevelProvider(
    const std::vector<std::vector<int>>& mapGridLayer1,
    const std::vector<std::vector<int>>& mapGridLayer2,
    const std::vector<std::vector<MapObjectId>>& mapObjectGrid,
    Texture2D floorTileset,
    Texture2D wallTileset,
    int& gridRows,
    int& gridCols
) : mapGridLayer1(mapGridLayer1), mapGridLayer2(mapGridLayer2),
    mapObjectGrid(mapObjectGrid), floorTileset(floorTileset),
    wallTileset(wallTileset), gridRows(gridRows), gridCols(gridCols) {}

int StaticLevelProvider::pos_hash(int x, int y) const {
    return x * 73856093 ^ y * 19349663;
}

bool StaticLevelProvider::IsSolidMapObject(MapObjectId objectId) const {
    if (objectId == MapObjectId::DestructibleBox || objectId == MapObjectId::Prop1 || objectId == MapObjectId::Prop2 || objectId == MapObjectId::MockWall) {
        return true; 
    }
    return false;
}

void StaticLevelProvider::DrawBase() {
    Rectangle wallTopSrc[2] = { {0.1f, 0.1f, 15.8f, 15.8f}, {16.1f, 0.1f, 15.8f, 15.8f} };
    Rectangle wallFrontFaceSrc[2] = { {0.1f, 16.1f, 15.8f, 15.8f}, {16.1f, 16.1f, 15.8f, 15.8f} };
    Rectangle floorSrc[6];
    for(int i = 0; i < 6; ++i) {
        floorSrc[i] = { (float)(i * 16) + 0.1f, 0.1f, 15.8f, 15.8f };
    }
    
    for (int layer = 1; layer <= 2; ++layer) {
        const auto& currentGrid = (layer == 1) ? mapGridLayer1 : mapGridLayer2;
        if (currentGrid.empty()) continue;

        // Draw Floors pass
        for (int r = -DRAW_PADDING_TILES; r < gridRows + DRAW_PADDING_TILES; ++r) {
            for (int c = -DRAW_PADDING_TILES; c < gridCols + DRAW_PADDING_TILES; ++c) {
                if (r >= 0 && c >= 0 && r < gridRows && c < (int)currentGrid[r].size()) {
                    int tileID = currentGrid[r][c];
                    // Floor tiles: positive IDs that are NOT wall IDs (0, 4-11)
                    bool isWallTile = (tileID == 0 || (tileID >= 4 && tileID <= 11));
                    if (tileID > 0 && !isWallTile) {
                        Rectangle destRec = {
                            std::floor((float)c * Constants::RENDER_TILE_SIZE),
                            std::floor((float)r * Constants::RENDER_TILE_SIZE),
                            Constants::RENDER_TILE_SIZE,
                            Constants::RENDER_TILE_SIZE
                        };
                        int variant = std::abs(pos_hash(c, r)) % 6;
                        DrawTexturePro(floorTileset, floorSrc[variant], destRec, {0,0}, 0.0f, WHITE);
                    }
                }
            }
        }
        
        // Draw Walls Front Faces pass
        for (int r = -DRAW_PADDING_TILES; r < gridRows + DRAW_PADDING_TILES; ++r) {
            for (int c = -DRAW_PADDING_TILES; c < gridCols + DRAW_PADDING_TILES; ++c) {
                if (r >= 0 && c >= 0 && r < gridRows && c < (int)currentGrid[r].size()) {
                    int tileID = currentGrid[r][c];
                    if (tileID == 0 || (tileID >= 4 && tileID <= 11)) {
                        Rectangle destRec = {
                            std::floor((float)c * Constants::RENDER_TILE_SIZE),
                            std::floor((float)r * Constants::RENDER_TILE_SIZE),
                            Constants::RENDER_TILE_SIZE,
                            Constants::RENDER_TILE_SIZE
                        };
                        int variant = std::abs(pos_hash(c, r)) % 2;
                        bool tileBelowIsFloor = true; 
                        if (r + 1 >= 0 && r + 1 < gridRows && c >= 0 && c < (int)currentGrid[r+1].size()) {
                            int belowTid = currentGrid[r+1][c];
                            if (belowTid == 0 || (belowTid >= 4 && belowTid <= 11)) tileBelowIsFloor = false; 
                        }
                        if (tileBelowIsFloor) {
                            Rectangle destRecFace = {
                                std::floor((float)c * Constants::RENDER_TILE_SIZE),
                                std::floor((float)(r + 1) * Constants::RENDER_TILE_SIZE),
                                Constants::RENDER_TILE_SIZE,
                                Constants::RENDER_TILE_SIZE
                            };
                            DrawTexturePro(wallTileset, wallFrontFaceSrc[variant], destRecFace, {0,0}, 0.0f, WHITE);
                        }
                    }
                }
            }
        }
    }
}

void StaticLevelProvider::GetDepthRenderItems(std::vector<DepthRenderItem>& items) {
    Rectangle wallTopSrc[2] = { {0, 0, 16, 16}, {16, 0, 16, 16} };
    for (int layer = 1; layer <= 2; ++layer) {
        const auto& currentGrid = (layer == 1) ? mapGridLayer1 : mapGridLayer2;
        if (currentGrid.empty()) continue;
        for (int r = -DRAW_PADDING_TILES; r < gridRows + DRAW_PADDING_TILES; ++r) {
            for (int c = -DRAW_PADDING_TILES; c < gridCols + DRAW_PADDING_TILES; ++c) {
                bool isWall = false;
                if (r >= 0 && c >= 0 && r < gridRows && c < (int)currentGrid[r].size()) {
                    int tid = currentGrid[r][c];
                    if (tid == 0 || (tid >= 4 && tid <= 11)) isWall = true;
                }
                if (isWall) {
                    float ySort = std::floor((float)(r + 1) * Constants::RENDER_TILE_SIZE);
                    items.push_back({
                        ySort,
                        [this, c, r, wallTopSrc]() {
                            Rectangle destRec = {
                                std::floor((float)c * Constants::RENDER_TILE_SIZE),
                                std::floor((float)r * Constants::RENDER_TILE_SIZE),
                                Constants::RENDER_TILE_SIZE,
                                Constants::RENDER_TILE_SIZE
                            };
                            int variant = std::abs(pos_hash(c, r)) % 2;
                            DrawTexturePro(wallTileset, wallTopSrc[variant], destRec, {0,0}, 0.0f, WHITE);
                        }
                    });
                }
            }
        }
    }
}

bool StaticLevelProvider::IsSolidCollision(Rectangle box, bool ignoreProps) const {
    int minCol = (int)std::floor((box.x + COLLISION_EDGE_PADDING) / Constants::RENDER_TILE_SIZE);
    int maxCol = (int)std::floor((box.x + box.width - COLLISION_EDGE_PADDING) / Constants::RENDER_TILE_SIZE);
    int minRow = (int)std::floor((box.y + COLLISION_EDGE_PADDING) / Constants::RENDER_TILE_SIZE);
    int maxRow = (int)std::floor((box.y + box.height - COLLISION_EDGE_PADDING) / Constants::RENDER_TILE_SIZE);

    for (int r = minRow; r <= maxRow; ++r) {
        for (int c = minCol; c <= maxCol; ++c) {
            int tileID1 = -1;
            int tileID2 = -1;
            MapObjectId mapObjectId = MapObjectId::Empty;
            if (r >= 0 && c >= 0 && r < gridRows && c < gridCols) {
                if (r < (int)mapGridLayer1.size() && c < (int)mapGridLayer1[r].size()) tileID1 = mapGridLayer1[r][c];
                if (r < (int)mapGridLayer2.size() && c < (int)mapGridLayer2[r].size()) tileID2 = mapGridLayer2[r][c];
                if (r < (int)mapObjectGrid.size() && c < (int)mapObjectGrid[r].size()) {
                    mapObjectId = mapObjectGrid[r][c];
                }
            } else {
                return true; 
            }

            int tileIDs[] = {tileID1, tileID2};
            for (int tileID : tileIDs) {
                if (tileID == 0) {
                    return true; 
                } else if (tileID >= 4 && tileID <= 11) {
                    return true; 
                } else if (tileID >= 1 && tileID <= 3) {
                    Rectangle solidPart = {
                        (float)c * Constants::RENDER_TILE_SIZE + RIGHT_PARTIAL_WALL_OFFSET,
                        (float)r * Constants::RENDER_TILE_SIZE,
                        PARTIAL_WALL_WIDTH,
                        Constants::RENDER_TILE_SIZE
                    };
                    if (CheckCollisionRecs(box, solidPart)) {
                        return true;
                    }
                } else if (tileID >= 12 && tileID <= 14) {
                    Rectangle solidPart = {
                        (float)c * Constants::RENDER_TILE_SIZE,
                        (float)r * Constants::RENDER_TILE_SIZE,
                        PARTIAL_WALL_WIDTH,
                        Constants::RENDER_TILE_SIZE
                    };
                    if (CheckCollisionRecs(box, solidPart)) {
                        return true;
                    }
                }
            }

            if (mapObjectId == MapObjectId::DestructibleBox || mapObjectId == MapObjectId::Prop1 || mapObjectId == MapObjectId::Prop2) {
                
            } else if (IsSolidMapObject(mapObjectId)) {
                return true;
            }
        }
    }
    return false;
}
