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
    
    int roomsToGenerate = 6;
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
                RoomType type = (generatedNodes.size() == roomsToGenerate - 1) ? RoomType::BOSS : RoomType::BATTLE;
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
    
    int roomOuterSize = Constants::ROOM_TILE_SIZE + Constants::CORRIDOR_LENGTH;
    int totalTilesW = gridW * roomOuterSize;
    int totalTilesH = gridH * roomOuterSize;
    
    auto baked = std::make_shared<RoomTemplate>(totalTilesW, totalTilesH);
    
    // Initialize everything to void (2)
    for (int y = 0; y < totalTilesH; ++y) {
        for (int x = 0; x < totalTilesW; ++x) {
            baked->layer0_tiles[y][x] = 2; // Pit / Void
        }
    }
    
    int roomCenter = Constants::ROOM_TILE_SIZE / 2;
    float tileW = Constants::TILE_SIZE * Constants::GLOBAL_SCALE;
    
    for (auto& node : generatedNodes) {
        int startX = node->gridX * roomOuterSize;
        int startY = node->gridY * roomOuterSize;
        
        // 1. Carve the room itself
        for (int y = 0; y < Constants::ROOM_TILE_SIZE; ++y) {
            for (int x = 0; x < Constants::ROOM_TILE_SIZE; ++x) {
                int px = startX + x;
                int py = startY + y;
                
                bool isWall = (x == 0 || x == Constants::ROOM_TILE_SIZE - 1 || y == 0 || y == Constants::ROOM_TILE_SIZE - 1);
                
                // Punch holes for corridors
                if (isWall) {
                    if (y == 0 && node->north && std::abs(x - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) isWall = false;
                    if (y == Constants::ROOM_TILE_SIZE - 1 && node->south && std::abs(x - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) isWall = false;
                    if (x == 0 && node->west && std::abs(y - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) isWall = false;
                    if (x == Constants::ROOM_TILE_SIZE - 1 && node->east && std::abs(y - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) isWall = false;
                }
                
                if (isWall) {
                    baked->layer0_tiles[py][px] = 1; // Wall
                } else {
                    baked->layer0_tiles[py][px] = 0; // Floor
                    
                    // Add Door anchors where corridors meet walls
                    if (x == roomCenter && y == 0 && node->north) baked->layer1_objects[py][px] = 20;
                    if (x == roomCenter && y == Constants::ROOM_TILE_SIZE - 1 && node->south) baked->layer1_objects[py][px] = 20;
                    if (y == roomCenter && x == 0 && node->west) baked->layer1_objects[py][px] = 20;
                    if (y == roomCenter && x == Constants::ROOM_TILE_SIZE - 1 && node->east) baked->layer1_objects[py][px] = 20;
                }
            }
        }
        
        // Compute trigger bounds for lockdown (inset by 2 tiles)
        node->triggerBounds = {
            (startX + 2.0f) * tileW,
            (startY + 2.0f) * tileW,
            (Constants::ROOM_TILE_SIZE - 4.0f) * tileW,
            (Constants::ROOM_TILE_SIZE - 4.0f) * tileW
        };
        
        // 2. Carve Corridors (only cast East and South to prevent double carving)
        if (node->east) {
            for (int y = roomCenter - Constants::CORRIDOR_WIDTH / 2; y <= roomCenter + Constants::CORRIDOR_WIDTH / 2; ++y) {
                for (int x = Constants::ROOM_TILE_SIZE; x < roomOuterSize; ++x) {
                    int px = startX + x;
                    int py = startY + y;
                    bool isWall = (y == roomCenter - Constants::CORRIDOR_WIDTH / 2 || y == roomCenter + Constants::CORRIDOR_WIDTH / 2);
                    baked->layer0_tiles[py][px] = isWall ? 1 : 0;
                }
            }
        }
        
        if (node->south) {
            for (int x = roomCenter - Constants::CORRIDOR_WIDTH / 2; x <= roomCenter + Constants::CORRIDOR_WIDTH / 2; ++x) {
                for (int y = Constants::ROOM_TILE_SIZE; y < roomOuterSize; ++y) {
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

void TilemapRenderer::DrawRoom(const RoomTemplate& room, Vector2 roomOffsetWorldPos, Texture2D tileset) {
    float tileSize = 32.0f; 
    float scaledTileSize = tileSize * Constants::GLOBAL_SCALE;

    for (int y = 0; y < room.height; ++y) {
        for (int x = 0; x < room.width; ++x) {
            int tileType = room.layer0_tiles[y][x];
            int objType = room.layer1_objects[y][x];
            
            Rectangle destRec = {
                std::floor(roomOffsetWorldPos.x + x * scaledTileSize),
                std::floor(roomOffsetWorldPos.y + y * scaledTileSize),
                scaledTileSize,
                scaledTileSize
            };
            
            if (tileType == 2) {
                continue; // Pit/void - skip entirely to save performance!
            } else if (tileType == 1 || objType == 20) { // Wall or door
                DrawRectangleRec(destRec, BLACK);
            } else { // Floor
                DrawRectangleRec(destRec, WHITE);
            }
        }
    }
}
