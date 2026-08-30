#include "Core/Level/StaticLevelProvider.h"
#include "Core/Level/VisibleWorld.h"
#include <cmath>
#include <algorithm>

namespace {
    constexpr float COLLISION_EDGE_PADDING = 0.001f;
    constexpr float PARTIAL_WALL_WIDTH = 9.0f;
    constexpr float RIGHT_PARTIAL_WALL_OFFSET = Constants::RENDER_TILE_SIZE - PARTIAL_WALL_WIDTH;
}

/// Creates a StaticLevelProvider instance from the supplied configuration.
StaticLevelProvider::StaticLevelProvider(
    const std::vector<std::vector<int>>& mapGridLayer1,
    const std::vector<std::vector<int>>& mapGridLayer2,
    Texture2D floorTileset,
    Texture2D wallTileset,
    int& gridRows,
    int& gridCols
) : mapGridLayer1(mapGridLayer1), mapGridLayer2(mapGridLayer2),
    floorTileset(floorTileset),
    wallTileset(wallTileset), gridRows(gridRows), gridCols(gridCols) {}

/// Implements the pos hash behavior for this component.
int StaticLevelProvider::pos_hash(int x, int y) const {
    return x * 73856093 ^ y * 19349663;
}

/// Renders base.
void StaticLevelProvider::DrawBase() {
    VisibleTileRange visible = GetVisibleTileRange(
        { 0.0f, 0.0f },
        gridCols,
        gridRows
    );
    if (visible.IsEmpty()) return;
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
        for (int r = visible.minimumY; r <= visible.maximumY; ++r) {
            for (int c = visible.minimumX; c <= visible.maximumX; ++c) {
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
        for (int r = visible.minimumY; r <= visible.maximumY; ++r) {
            for (int c = visible.minimumX; c <= visible.maximumX; ++c) {
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

/// Returns the current depth render items.
void StaticLevelProvider::GetDepthRenderItems(std::vector<DepthRenderItem>& items) {
    VisibleTileRange visible = GetVisibleTileRange(
        { 0.0f, 0.0f },
        gridCols,
        gridRows
    );
    if (visible.IsEmpty()) return;
    Rectangle wallTopSrc[2] = { {0, 0, 16, 16}, {16, 0, 16, 16} };
    for (int layer = 1; layer <= 2; ++layer) {
        const auto& currentGrid = (layer == 1) ? mapGridLayer1 : mapGridLayer2;
        if (currentGrid.empty()) continue;
        for (int r = visible.minimumY; r <= visible.maximumY; ++r) {
            for (int c = visible.minimumX; c <= visible.maximumX; ++c) {
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

/// Reports whether the solid collision condition is satisfied.
bool StaticLevelProvider::IsSolidCollision(Rectangle box) const {
    int minCol = (int)std::floor((box.x + COLLISION_EDGE_PADDING) / Constants::RENDER_TILE_SIZE);
    int maxCol = (int)std::floor((box.x + box.width - COLLISION_EDGE_PADDING) / Constants::RENDER_TILE_SIZE);
    int minRow = (int)std::floor((box.y + COLLISION_EDGE_PADDING) / Constants::RENDER_TILE_SIZE);
    int maxRow = (int)std::floor((box.y + box.height - COLLISION_EDGE_PADDING) / Constants::RENDER_TILE_SIZE);

    for (int r = minRow; r <= maxRow; ++r) {
        for (int c = minCol; c <= maxCol; ++c) {
            int tileID1 = -1;
            int tileID2 = -1;
            if (r >= 0 && c >= 0 && r < gridRows && c < gridCols) {
                if (r < (int)mapGridLayer1.size() && c < (int)mapGridLayer1[r].size()) tileID1 = mapGridLayer1[r][c];
                if (r < (int)mapGridLayer2.size() && c < (int)mapGridLayer2[r].size()) tileID2 = mapGridLayer2[r][c];
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
        }
    }
    return false;
}

/// Appends static blocking colliders for tile.
void StaticLevelProvider::AppendStaticBlockingCollidersForTile(
    int tileX,
    int tileY,
    std::vector<Rectangle>& output
) const {
    Rectangle fullTile = {
        tileX * Constants::RENDER_TILE_SIZE,
        tileY * Constants::RENDER_TILE_SIZE,
        Constants::RENDER_TILE_SIZE,
        Constants::RENDER_TILE_SIZE
    };
    if (tileX < 0 || tileY < 0 ||
        tileX >= gridCols || tileY >= gridRows) {
        output.push_back(fullTile);
        return;
    }

    int tileId1 = -1;
    int tileId2 = -1;
    if (tileY < (int)mapGridLayer1.size() &&
        tileX < (int)mapGridLayer1[tileY].size()) {
        tileId1 = mapGridLayer1[tileY][tileX];
    }
    if (tileY < (int)mapGridLayer2.size() &&
        tileX < (int)mapGridLayer2[tileY].size()) {
        tileId2 = mapGridLayer2[tileY][tileX];
    }

    int tileIds[2] = { tileId1, tileId2 };
    for (int tileId : tileIds) {
        if (tileId == 0 || (tileId >= 4 && tileId <= 11)) {
            output.push_back(fullTile);
            return;
        }
    }

    auto appendUnique = [&output](Rectangle collider) {
        for (Rectangle existing : output) {
            if (existing.x == collider.x && existing.y == collider.y &&
                existing.width == collider.width &&
                existing.height == collider.height) {
                return;
            }
        }
        output.push_back(collider);
    };

    for (int tileId : tileIds) {
        if (tileId >= 1 && tileId <= 3) {
            appendUnique({
                fullTile.x + RIGHT_PARTIAL_WALL_OFFSET,
                fullTile.y,
                PARTIAL_WALL_WIDTH,
                fullTile.height
            });
        } else if (tileId >= 12 && tileId <= 14) {
            appendUnique({
                fullTile.x,
                fullTile.y,
                PARTIAL_WALL_WIDTH,
                fullTile.height
            });
        }
    }

}
