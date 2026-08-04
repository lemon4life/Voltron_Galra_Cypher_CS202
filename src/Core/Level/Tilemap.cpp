#include "Core/Level/Tilemap.h"
#include "Core/Constants.h"
#include <cmath>
#include <iostream>

std::vector<Rectangle> RoomTemplate::GenerateWallColliders(Vector2 offsetWorldPos, float tileSize, float scale) const {
    std::vector<Rectangle> colliders;
    float scaledTileSize = tileSize * scale;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (layer0_tiles[y][x] == 1) { // 1 is wall
                Rectangle rect = {
                    offsetWorldPos.x + x * scaledTileSize,
                    offsetWorldPos.y + y * scaledTileSize,
                    scaledTileSize,
                    scaledTileSize
                };
                colliders.push_back(rect);
            }
        }
    }
    return colliders;
}

void LevelMap::Generate(int width, int height) {
    grid.assign(height, std::vector<std::shared_ptr<RoomNode>>(width, nullptr));
    generatedNodes.clear();
    
    int cx = width / 2;
    int cy = height / 2;
    spawnRoom = std::make_shared<RoomNode>(cx, cy, RoomType::SPAWN);
    spawnRoom->isDiscovered = true;
    spawnRoom->isCleared = true;
    spawnRoom->state = RoomState::CLEARED;
    grid[cy][cx] = spawnRoom;
    generatedNodes.push_back(spawnRoom);
    
    int roomsToGenerate = 7;
    int currentX = cx;
    int currentY = cy;
    
    std::shared_ptr<RoomNode> currentNode = spawnRoom;
    
    while (generatedNodes.size() < roomsToGenerate) {
        // Pick random direction
        int dir = GetRandomValue(0, 3);
        int dx = 0, dy = 0;
        if (dir == 0) dy = -1; // North
        else if (dir == 1) dy = 1; // South
        else if (dir == 2) dx = 1; // East
        else dx = -1; // West
        
        int nx = currentX + dx;
        int ny = currentY + dy;
        
        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            std::shared_ptr<RoomNode> nextNode = grid[ny][nx];
            if (!nextNode) {
                RoomType type = RoomType::BATTLE;
                if (generatedNodes.size() == roomsToGenerate - 2) type = RoomType::BOSS;
                else if (generatedNodes.size() == roomsToGenerate - 1) type = RoomType::EXIT;
                nextNode = std::make_shared<RoomNode>(nx, ny, type);
                grid[ny][nx] = nextNode;
                generatedNodes.push_back(nextNode);
            }
            
            // Link them
            if (dy == -1) { currentNode->north = nextNode; nextNode->south = currentNode; }
            if (dy == 1) { currentNode->south = nextNode; nextNode->north = currentNode; }
            if (dx == 1) { currentNode->east = nextNode; nextNode->west = currentNode; }
            if (dx == -1) { currentNode->west = nextNode; nextNode->east = currentNode; }
            
            currentX = nx;
            currentY = ny;
            currentNode = nextNode;
        }
    }
}

std::shared_ptr<RoomTemplate> LevelMap::BakeLevel() {
    if (grid.empty() || generatedNodes.empty()) return nullptr;
    
    int gridW = grid[0].size();
    int gridH = grid.size();
    
    int roomOuterSize = Constants::MAX_ROOM_TILE_SIZE + Constants::CORRIDOR_LENGTH;
    int totalTilesW = gridW * roomOuterSize;
    int totalTilesH = gridH * roomOuterSize;
    
    auto baked = std::make_shared<RoomTemplate>(totalTilesW, totalTilesH);
    
    // Initialize everything to void (2)
    for (int y = 0; y < totalTilesH; ++y) {
        for (int x = 0; x < totalTilesW; ++x) {
            baked->layer0_tiles[y][x] = 2; // Pit / Void
        }
    }
    
    int roomCenter = Constants::MAX_ROOM_TILE_SIZE / 2;
    float tileW = Constants::TILE_SIZE;
    
    for (auto& node : generatedNodes) {
        int startX = node->gridX * roomOuterSize;
        int startY = node->gridY * roomOuterSize;
        
        int currentRoomSize = (node->type == RoomType::BATTLE || node->type == RoomType::BOSS)
                              ? Constants::MAX_ROOM_TILE_SIZE
                              : Constants::NORMAL_ROOM_TILE_SIZE;
        
        int offset = (Constants::MAX_ROOM_TILE_SIZE - currentRoomSize) / 2;
        
        // 1. Carve the room itself
        for (int y = 0; y < currentRoomSize; ++y) {
            for (int x = 0; x < currentRoomSize; ++x) {
                int px = startX + offset + x;
                int py = startY + offset + y;
                
                bool isWall = (x == 0 || x == currentRoomSize - 1 || y == 0 || y == currentRoomSize - 1);
                
                // Punch holes for corridors
                if (isWall) {
                    if (y == 0 && node->north && std::abs((x + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) isWall = false;
                    if (y == currentRoomSize - 1 && node->south && std::abs((x + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) isWall = false;
                    if (x == 0 && node->west && std::abs((y + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) isWall = false;
                    if (x == currentRoomSize - 1 && node->east && std::abs((y + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) isWall = false;
                }
                
                if (isWall) {
                    baked->layer0_tiles[py][px] = 1; // Wall
                } else {
                    baked->layer0_tiles[py][px] = 0; // Floor
                    // Add Door anchors where corridors meet walls
                    if (y == 0 && node->north && std::abs((x + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) baked->layer1_objects[py][px] = 20;
                    if (y == currentRoomSize - 1 && node->south && std::abs((x + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) baked->layer1_objects[py][px] = 20;
                    if (x == 0 && node->west && std::abs((y + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) baked->layer1_objects[py][px] = 20;
                    if (x == currentRoomSize - 1 && node->east && std::abs((y + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) baked->layer1_objects[py][px] = 20;
                }
            }
        }
        
        // Compute trigger bounds for lockdown (inset by 2 tiles)
        node->triggerBounds = {
            (startX + offset + 2.0f) * tileW,
            (startY + offset + 2.0f) * tileW,
            (currentRoomSize - 4.0f) * tileW,
            (currentRoomSize - 4.0f) * tileW
        };
        
        // 2. Carve Corridors
        if (node->east) {
            for (int y = roomCenter - Constants::CORRIDOR_WIDTH / 2; y <= roomCenter + Constants::CORRIDOR_WIDTH / 2; ++y) {
                for (int x = offset + currentRoomSize; x < roomOuterSize; ++x) {
                    int px = startX + x;
                    int py = startY + y;
                    bool isWall = (y == roomCenter - Constants::CORRIDOR_WIDTH / 2 || y == roomCenter + Constants::CORRIDOR_WIDTH / 2);
                    baked->layer0_tiles[py][px] = isWall ? 1 : 0;
                }
            }
        }
        
        if (node->south) {
            for (int x = roomCenter - Constants::CORRIDOR_WIDTH / 2; x <= roomCenter + Constants::CORRIDOR_WIDTH / 2; ++x) {
                for (int y = offset + currentRoomSize; y < roomOuterSize; ++y) {
                    int px = startX + x;
                    int py = startY + y;
                    bool isWall = (x == roomCenter - Constants::CORRIDOR_WIDTH / 2 || x == roomCenter + Constants::CORRIDOR_WIDTH / 2);
                    baked->layer0_tiles[py][px] = isWall ? 1 : 0;
                }
            }
        }
        
        if (node->west) {
            for (int y = roomCenter - Constants::CORRIDOR_WIDTH / 2; y <= roomCenter + Constants::CORRIDOR_WIDTH / 2; ++y) {
                for (int x = 0; x < offset; ++x) {
                    int px = startX + x;
                    int py = startY + y;
                    bool isWall = (y == roomCenter - Constants::CORRIDOR_WIDTH / 2 || y == roomCenter + Constants::CORRIDOR_WIDTH / 2);
                    baked->layer0_tiles[py][px] = isWall ? 1 : 0;
                }
            }
        }
        
        if (node->north) {
            for (int x = roomCenter - Constants::CORRIDOR_WIDTH / 2; x <= roomCenter + Constants::CORRIDOR_WIDTH / 2; ++x) {
                for (int y = 0; y < offset; ++y) {
                    int px = startX + x;
                    int py = startY + y;
                    bool isWall = (x == roomCenter - Constants::CORRIDOR_WIDTH / 2 || x == roomCenter + Constants::CORRIDOR_WIDTH / 2);
                    baked->layer0_tiles[py][px] = isWall ? 1 : 0;
                }
            }
        }
    }
    
    return baked;
}

void TilemapRenderer::DrawRoom(const RoomTemplate& room, Vector2 roomOffsetWorldPos, Texture2D floorTileset, Texture2D wallTileset) {
    float scaledTileSize = Constants::RENDER_TILE_SIZE;

    Rectangle wallTopSrc[2] = { {0, 0, 16, 16}, {16, 0, 16, 16} };
    Rectangle wallFrontFaceSrc[2] = { {0, 16, 16, 16}, {16, 16, 16, 16} };
    
    Rectangle floorSrc[6];
    for(int i = 0; i < 6; ++i) {
        floorSrc[i] = { (float)(i * 16), 0.0f, 16.0f, 16.0f };
    }

    auto hash = [](int x, int y) -> int {
        unsigned int h = (unsigned int)(x * 374761393 ^ y * 668265263);
        h = (h ^ (h >> 13)) * 1274126177;
        return h ^ (h >> 16);
    };

    // Draw Floors
    for (int y = 0; y < room.height; ++y) {
        for (int x = 0; x < room.width; ++x) {
            int tileType = room.layer0_tiles[y][x];
            int objType = room.layer1_objects[y][x];
            
            if (tileType == 2 || tileType == 1 || objType == 20) continue; 
            
            Rectangle destRec = {
                std::floor(roomOffsetWorldPos.x + x * scaledTileSize),
                std::floor(roomOffsetWorldPos.y + y * scaledTileSize),
                scaledTileSize,
                scaledTileSize
            };
            int variant = std::abs(hash(x, y)) % 6;
            DrawTexturePro(floorTileset, floorSrc[variant], destRec, {0,0}, 0.0f, WHITE);
        }
    }

    // Draw Walls and Depth
    for (int y = 0; y < room.height; ++y) {
        for (int x = 0; x < room.width; ++x) {
            int tileType = room.layer0_tiles[y][x];
            int objType = room.layer1_objects[y][x];
            
            if (tileType == 1 || objType == 20) {
                Rectangle destRec = {
                    std::floor(roomOffsetWorldPos.x + x * scaledTileSize),
                    std::floor(roomOffsetWorldPos.y + y * scaledTileSize),
                    scaledTileSize,
                    scaledTileSize
                };
                int variant = std::abs(hash(x, y)) % 2;
                DrawTexturePro(wallTileset, wallTopSrc[variant], destRec, {0,0}, 0.0f, WHITE);
                
                // Depth logic: if tile below is floor or void
                bool tileBelowIsFloorOrVoid = false;
                if (y + 1 >= room.height) {
                    tileBelowIsFloorOrVoid = true;
                } else {
                    int typeBelow = room.layer0_tiles[y+1][x];
                    if (typeBelow == 0 || typeBelow == 2) {
                        tileBelowIsFloorOrVoid = true;
                    }
                }
                
                if (tileBelowIsFloorOrVoid && (y + 1 >= room.height || room.layer1_objects[y+1][x] != 20)) {
                    Rectangle destRecFace = {
                        std::floor(roomOffsetWorldPos.x + x * scaledTileSize),
                        std::floor(roomOffsetWorldPos.y + (y + 1) * scaledTileSize),
                        scaledTileSize,
                        scaledTileSize
                    };
                    DrawTexturePro(wallTileset, wallFrontFaceSrc[variant], destRecFace, {0,0}, 0.0f, WHITE);
                }
            }
        }
    }
}
