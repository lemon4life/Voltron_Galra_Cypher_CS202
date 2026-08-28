#include "Core/Level/Tilemap.h"
#include "Core/Constants.h"
#include <cmath>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <queue>
#include <unordered_map>

std::vector<Rectangle> RoomTemplate::GenerateWallColliders(Vector2 offsetWorldPos, float tileSize, float scale) const {
    std::vector<Rectangle> colliders;
    float scaledTileSize = tileSize * scale;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (layer0_tiles[y][x] == 1) { // Only explicit wall tiles (NOT void/2)
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

void LevelMap::Generate(
    int width,
    int height,
    int enemyRoomCount,
    int chestRoomCount,
    int enhanceRoomCount,
    bool bossFloor
) {
    enemyRoomCount = std::max(1, enemyRoomCount);
    chestRoomCount = std::clamp(chestRoomCount, 0, 2);
    enhanceRoomCount = std::clamp(enhanceRoomCount, 0, 1);

    struct OpenSlot {
        int x;
        int y;
        std::shared_ptr<RoomNode> parent;
        int dx;
        int dy;
    };

    auto setRoomType = [](const std::shared_ptr<RoomNode>& node,
                          RoomType type) {
        node->type = type;
        node->roomSize = type == RoomType::BOSS
            ? 25
            : (type == RoomType::BATTLE ? 20 : 15);
        bool utility = type == RoomType::SPAWN ||
            type == RoomType::CHEST || type == RoomType::EVENT ||
            type == RoomType::EXIT;
        node->isCleared = utility;
        node->state = utility ? RoomState::CLEARED : RoomState::IDLE;
    };

    constexpr int MAX_GENERATION_ATTEMPTS = 32;
    for (int attempt = 0; attempt < MAX_GENERATION_ATTEMPTS; ++attempt) {
        grid.assign(
            height,
            std::vector<std::shared_ptr<RoomNode>>(width, nullptr)
        );
        generatedNodes.clear();

        int centerX = width / 2;
        int centerY = height / 2;
        spawnRoom = std::make_shared<RoomNode>(
            centerX,
            centerY,
            RoomType::SPAWN
        );
        setRoomType(spawnRoom, RoomType::SPAWN);
        spawnRoom->isDiscovered = true;
        grid[centerY][centerX] = spawnRoom;
        generatedNodes.push_back(spawnRoom);

        auto collectOpenSlots = [&]() {
            std::vector<OpenSlot> slots;
            for (const auto& node : generatedNodes) {
                int x = node->gridX;
                int y = node->gridY;
                if (y > 0 && !grid[y - 1][x]) {
                    slots.push_back({ x, y - 1, node, 0, -1 });
                }
                if (y + 1 < height && !grid[y + 1][x]) {
                    slots.push_back({ x, y + 1, node, 0, 1 });
                }
                if (x > 0 && !grid[y][x - 1]) {
                    slots.push_back({ x - 1, y, node, -1, 0 });
                }
                if (x + 1 < width && !grid[y][x + 1]) {
                    slots.push_back({ x + 1, y, node, 1, 0 });
                }
            }
            return slots;
        };

        auto connectRoom = [&](const OpenSlot& slot, RoomType type) {
            auto node = std::make_shared<RoomNode>(slot.x, slot.y, type);
            grid[slot.y][slot.x] = node;
            generatedNodes.push_back(node);
            if (slot.dx == 1) {
                slot.parent->east = node.get();
                node->west = slot.parent.get();
            } else if (slot.dx == -1) {
                slot.parent->west = node.get();
                node->east = slot.parent.get();
            } else if (slot.dy == 1) {
                slot.parent->south = node.get();
                node->north = slot.parent.get();
            } else {
                slot.parent->north = node.get();
                node->south = slot.parent.get();
            }
            return node;
        };

        int roomsBeforePortal = enemyRoomCount + chestRoomCount +
            enhanceRoomCount + (bossFloor ? 0 : 1);
        bool failed = false;
        for (int index = 0; index < roomsBeforePortal; ++index) {
            std::vector<OpenSlot> slots = collectOpenSlots();
            if (slots.empty()) {
                failed = true;
                break;
            }
            connectRoom(
                slots[GetRandomValue(0, (int)slots.size() - 1)],
                RoomType::BATTLE
            );
        }
        if (failed) continue;

        std::unordered_map<RoomNode*, int> distances;
        std::unordered_map<RoomNode*, RoomNode*> parents;
        std::queue<RoomNode*> frontier;
        distances[spawnRoom.get()] = 0;
        parents[spawnRoom.get()] = nullptr;
        frontier.push(spawnRoom.get());

        while (!frontier.empty()) {
            RoomNode* current = frontier.front();
            frontier.pop();
            RoomNode* neighbors[] = {
                current->north,
                current->south,
                current->east,
                current->west
            };
            for (RoomNode* neighbor : neighbors) {
                if (!neighbor || distances.count(neighbor) > 0) continue;
                distances[neighbor] = distances[current] + 1;
                parents[neighbor] = current;
                frontier.push(neighbor);
            }
        }

        std::vector<std::shared_ptr<RoomNode>> farthestLeaves;
        int farthestDistance = -1;
        for (const auto& node : generatedNodes) {
            if (node == spawnRoom) continue;
            int connectionCount = (node->north ? 1 : 0) +
                (node->south ? 1 : 0) + (node->east ? 1 : 0) +
                (node->west ? 1 : 0);
            if (connectionCount != 1) continue;

            int distance = distances[node.get()];
            if (distance > farthestDistance) {
                farthestDistance = distance;
                farthestLeaves.clear();
            }
            if (distance == farthestDistance) {
                farthestLeaves.push_back(node);
            }
        }

        if (bossFloor) {
            farthestLeaves.erase(
                std::remove_if(
                    farthestLeaves.begin(),
                    farthestLeaves.end(),
                    [&](const std::shared_ptr<RoomNode>& node) {
                        RoomNode* parent = parents[node.get()];
                        int deltaX = node->gridX - parent->gridX;
                        int deltaY = node->gridY - parent->gridY;
                        int portalX = node->gridX + deltaX;
                        int portalY = node->gridY + deltaY;
                        return portalX < 0 || portalX >= width ||
                            portalY < 0 || portalY >= height ||
                            grid[portalY][portalX] != nullptr;
                    }
                ),
                farthestLeaves.end()
            );
        }
        if (farthestLeaves.empty()) continue;

        auto progressionRoom = farthestLeaves[
            GetRandomValue(0, (int)farthestLeaves.size() - 1)
        ];
        RoomNode* bossParent = nullptr;
        if (bossFloor) {
            setRoomType(progressionRoom, RoomType::BOSS);
            bossParent = parents[progressionRoom.get()];
            int deltaX = progressionRoom->gridX - bossParent->gridX;
            int deltaY = progressionRoom->gridY - bossParent->gridY;
            connectRoom(
                {
                    progressionRoom->gridX + deltaX,
                    progressionRoom->gridY + deltaY,
                    progressionRoom,
                    deltaX,
                    deltaY
                },
                RoomType::EXIT
            );
        } else {
            setRoomType(progressionRoom, RoomType::EXIT);
        }

        std::vector<std::shared_ptr<RoomNode>> utilityCandidates;
        for (const auto& node : generatedNodes) {
            if (node == spawnRoom || node->type == RoomType::EXIT ||
                node->type == RoomType::BOSS ||
                node.get() == bossParent) {
                continue;
            }
            utilityCandidates.push_back(node);
        }
        for (int index = (int)utilityCandidates.size() - 1;
             index > 0;
             --index) {
            int swapIndex = GetRandomValue(0, index);
            std::swap(utilityCandidates[index], utilityCandidates[swapIndex]);
        }

        int candidateIndex = 0;
        for (int index = 0; index < chestRoomCount; ++index) {
            setRoomType(
                utilityCandidates[candidateIndex++],
                RoomType::CHEST
            );
        }
        for (int index = 0; index < enhanceRoomCount; ++index) {
            setRoomType(
                utilityCandidates[candidateIndex++],
                RoomType::EVENT
            );
        }
        return;
    }

    printf("LevelMap::Generate: failed to build a valid room tree\n");
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
        
        int currentRoomSize = node->roomSize;
        
        int offset = (Constants::MAX_ROOM_TILE_SIZE - currentRoomSize) / 2;
        
        // For SPAWN room (which doesn't use CSV), carve a default walled room.
        // For all other rooms the CSV will overwrite this with its own content.
        for (int y = 0; y < currentRoomSize; ++y) {
            for (int x = 0; x < currentRoomSize; ++x) {
                int px = startX + offset + x;
                int py = startY + offset + y;
                // Default: floor with solid perimeter walls (CSV rooms overwrite)
                bool isWall = (x == 0 || x == currentRoomSize - 1 || y == 0 || y == currentRoomSize - 1);
                if (isWall) {
                    baked->layer0_tiles[py][px] = 1; // Wall
                } else {
                    baked->layer0_tiles[py][px] = 0; // Floor
                }
            }
        }
        
        // Spawn and Boss rooms use the fixed empty-room layout created above.
        // Other room types may load randomized CSV layouts.
        if (node->type != RoomType::SPAWN &&
            node->type != RoomType::BOSS) {
            std::string sizePrefix = "Small";
            if (node->roomSize == 25) sizePrefix = "Large";
            else if (node->roomSize == 20) sizePrefix = "Medium";
            
            std::vector<std::string> templates;
            if (node->roomSize == 15) {
                templates.push_back("assets/level/Small_01.csv");
            } else if (std::filesystem::exists("assets/level")) {
                for (const auto& entry : std::filesystem::directory_iterator("assets/level")) {
                    if (entry.is_regular_file() && entry.path().extension() == ".csv") {
                        std::string filename = entry.path().filename().string();
                        if (filename.find(sizePrefix + "_") == 0) {
                            templates.push_back(entry.path().string());
                        }
                    }
                }
            }
            
            if (!templates.empty()) {
                std::string selectedTemplate = templates[GetRandomValue(0, templates.size() - 1)];
                std::ifstream file(selectedTemplate);
                if (file.is_open()) {
                    std::string line;
                    std::getline(file, line); // Skip header
                        // We will carve the floor based on the room size, since CSV is exactly that size
                        for (int y = 0; y < currentRoomSize; ++y) {
                            for (int x = 0; x < currentRoomSize; ++x) {
                                int px = startX + offset + x;
                                int py = startY + offset + y;
                                baked->layer0_tiles[py][px] = 0; // Floor
                            }
                        }
                        
                        while (std::getline(file, line)) {
                            if (line.empty() || line.back() == '\r') line.pop_back();
                            std::stringstream ss(line);
                            std::string token;
                            int objId = -1, gx = -1, gy = -1;
                            
                            if (std::getline(ss, token, ',')) objId = std::stoi(token);
                            if (std::getline(ss, token, ',')) gx = std::stoi(token);
                            if (std::getline(ss, token, ',')) gy = std::stoi(token);
                            
                            if (objId != -1 && gx >= 0 && gx < currentRoomSize && gy >= 0 && gy < currentRoomSize) {
                                int px = startX + offset + gx;
                                int py = startY + offset + gy;
                                
                                if (objId == 1) { // Wall
                                    baked->layer0_tiles[py][px] = 1;
                                } else if (objId == 2) { // Floor
                                    baked->layer0_tiles[py][px] = 0;
                                } else {
                                    baked->layer2_props[py][px] = objId; // Props and entities
                                }
                            }
                        }
                        
                        // If it is an EXIT room, ensure the central area under the huge transfer gate is completely clear of walls/props
                        if (node->type == RoomType::EXIT) {
                            for (int y = currentRoomSize / 2 - 3; y <= currentRoomSize / 2 + 3; ++y) {
                                for (int x = currentRoomSize / 2 - 3; x <= currentRoomSize / 2 + 3; ++x) {
                                    if (x >= 0 && x < currentRoomSize && y >= 0 && y < currentRoomSize) {
                                        int px = startX + offset + x;
                                        int py = startY + offset + y;
                                        baked->layer0_tiles[py][px] = 0; // Force floor
                                        baked->layer2_props[py][px] = 0; // Force no props
                                    }
                                }
                            }
                        }

                        // If it is a CHEST or EVENT room, clear layer2_props so only the animated entity is placed (Chest or EnhanceMachine)
                        if (node->type == RoomType::CHEST || node->type == RoomType::EVENT) {
                            for (int y = 0; y < currentRoomSize; ++y) {
                                for (int x = 0; x < currentRoomSize; ++x) {
                                    int px = startX + offset + x;
                                    int py = startY + offset + y;
                                    baked->layer2_props[py][px] = 0;
                                }
                            }
                        }
                        
                        // Dynamically pierce walls for corridors
                        // (extracted to lambda, called below for all room types)
                        
                        // After CSV loading, re-stamp the perimeter wall ring so the room always
                        // has a visible physical bounding wall, regardless of what the CSV defines.
                        for (int y = 0; y < currentRoomSize; ++y) {
                            for (int x = 0; x < currentRoomSize; ++x) {
                                int px = startX + offset + x;
                                int py = startY + offset + y;
                                bool isPerimeter = (x == 0 || x == currentRoomSize - 1 || y == 0 || y == currentRoomSize - 1);
                                if (isPerimeter) {
                                    baked->layer0_tiles[py][px] = 1; // Restore perimeter wall
                                    baked->layer2_props[py][px] = 0; // Remove any prop on the perimeter edge
                                }
                            }
                        }
                }
            }
        }

        if (node->type == RoomType::BOSS) {
            // Keep the 25x25 Boss arena completely open inside its perimeter.
            // Four 2x2 destructible-box groups sit halfway between the arena
            // center and its corners.
            for (int y = 1; y < currentRoomSize - 1; ++y) {
                for (int x = 1; x < currentRoomSize - 1; ++x) {
                    int px = startX + offset + x;
                    int py = startY + offset + y;
                    baked->layer0_tiles[py][px] = 0;
                    baked->layer1_objects[py][px] = 0;
                    baked->layer2_props[py][px] = 0;
                }
            }

            int nearSide = currentRoomSize / 4 - 1;
            int farSide = currentRoomSize - currentRoomSize / 4 - 2;
            constexpr int clusterSize = 2;
            const int clusterStarts[4][2] = {
                { nearSide, nearSide },
                { farSide, nearSide },
                { nearSide, farSide },
                { farSide, farSide }
            };
            for (const auto& clusterStart : clusterStarts) {
                for (int dy = 0; dy < clusterSize; ++dy) {
                    for (int dx = 0; dx < clusterSize; ++dx) {
                        int px = startX + offset + clusterStart[0] + dx;
                        int py = startY + offset + clusterStart[1] + dy;
                        baked->layer2_props[py][px] = 5;
                    }
                }
            }
        }

        // ---------------------------------------------------------------
        // Pierce room perimeter walls where corridors connect (ALL rooms)
        // This MUST run after CSV loading so all walls are in place.
        // ---------------------------------------------------------------
        {
            // Helper: clear a tile and remove any MockWall prop at that position
            auto clearTile = [&](int py, int px) {
                baked->layer0_tiles[py][px] = 0;
                if (baked->layer2_props[py][px] == 4) baked->layer2_props[py][px] = 0;
            };

            // Pierce NORTH wall and place DoorGate
            if (node->north) {
                // Scan from the north edge of the room inward until we hit and pass a wall row
                bool hitWall = false;
                int doorRow = -1;
                for (int y = 0; y <= currentRoomSize / 2; ++y) {
                    bool rowHasWall = false;
                    for (int x = 0; x < currentRoomSize; ++x) {
                        if (std::abs((x + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) {
                            int tile = baked->layer0_tiles[startY + offset + y][startX + offset + x];
                            int prop = baked->layer2_props[startY + offset + y][startX + offset + x];
                            if (tile == 1 || prop == 4) rowHasWall = true;
                        }
                    }
                    if (rowHasWall) {
                        hitWall = true;
                        doorRow = y;
                    }
                    if (!rowHasWall && hitWall) break; // just passed through the wall
                    // Clear corridor-width tiles on every row from top until through the wall
                    for (int x = 0; x < currentRoomSize; ++x) {
                        if (std::abs((x + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) {
                            clearTile(startY + offset + y, startX + offset + x);
                        }
                    }
                }
                // Place DoorGates across the FULL corridor width on the innermost wall row pierced
                int halfW = Constants::CORRIDOR_WIDTH / 2;
                if (doorRow >= 0) {
                    for (int dx = -halfW; dx <= halfW; ++dx) {
                        baked->layer1_objects[startY + offset + doorRow][startX + roomCenter + dx] = 20;
                    }
                } else {
                    // Room had no north wall — place DoorGates at the room's north edge
                    for (int dx = -halfW; dx <= halfW; ++dx) {
                        baked->layer1_objects[startY + offset][startX + roomCenter + dx] = 20;
                    }
                }
            }

            // Pierce SOUTH wall and place DoorGate
            if (node->south) {
                bool hitWall = false;
                int doorRow = -1;
                for (int y = currentRoomSize - 1; y >= currentRoomSize / 2; --y) {
                    bool rowHasWall = false;
                    for (int x = 0; x < currentRoomSize; ++x) {
                        if (std::abs((x + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) {
                            int tile = baked->layer0_tiles[startY + offset + y][startX + offset + x];
                            int prop = baked->layer2_props[startY + offset + y][startX + offset + x];
                            if (tile == 1 || prop == 4) rowHasWall = true;
                        }
                    }
                    if (rowHasWall) {
                        hitWall = true;
                        doorRow = y;
                    }
                    if (!rowHasWall && hitWall) break;
                    for (int x = 0; x < currentRoomSize; ++x) {
                        if (std::abs((x + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) {
                            clearTile(startY + offset + y, startX + offset + x);
                        }
                    }
                }
                int halfW2 = Constants::CORRIDOR_WIDTH / 2;
                if (doorRow >= 0) {
                    for (int dx = -halfW2; dx <= halfW2; ++dx) {
                        baked->layer1_objects[startY + offset + doorRow][startX + roomCenter + dx] = 20;
                    }
                } else {
                    for (int dx = -halfW2; dx <= halfW2; ++dx) {
                        baked->layer1_objects[startY + offset + currentRoomSize - 1][startX + roomCenter + dx] = 20;
                    }
                }
            }

            // Pierce WEST wall and place DoorGate
            if (node->west) {
                bool hitWall = false;
                int doorCol = -1;
                for (int x = 0; x <= currentRoomSize / 2; ++x) {
                    bool colHasWall = false;
                    for (int y = 0; y < currentRoomSize; ++y) {
                        if (std::abs((y + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) {
                            int tile = baked->layer0_tiles[startY + offset + y][startX + offset + x];
                            int prop = baked->layer2_props[startY + offset + y][startX + offset + x];
                            if (tile == 1 || prop == 4) colHasWall = true;
                        }
                    }
                    if (colHasWall) {
                        hitWall = true;
                        doorCol = x;
                    }
                    if (!colHasWall && hitWall) break;
                    for (int y = 0; y < currentRoomSize; ++y) {
                        if (std::abs((y + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) {
                            clearTile(startY + offset + y, startX + offset + x);
                        }
                    }
                }
                int halfH = Constants::CORRIDOR_WIDTH / 2;
                if (doorCol >= 0) {
                    for (int dy = -halfH; dy <= halfH; ++dy) {
                        baked->layer1_objects[startY + roomCenter + dy][startX + offset + doorCol] = 20;
                    }
                } else {
                    for (int dy = -halfH; dy <= halfH; ++dy) {
                        baked->layer1_objects[startY + roomCenter + dy][startX + offset] = 20;
                    }
                }
            }

            // Pierce EAST wall and place DoorGate
            if (node->east) {
                bool hitWall = false;
                int doorCol = -1;
                for (int x = currentRoomSize - 1; x >= currentRoomSize / 2; --x) {
                    bool colHasWall = false;
                    for (int y = 0; y < currentRoomSize; ++y) {
                        if (std::abs((y + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) {
                            int tile = baked->layer0_tiles[startY + offset + y][startX + offset + x];
                            int prop = baked->layer2_props[startY + offset + y][startX + offset + x];
                            if (tile == 1 || prop == 4) colHasWall = true;
                        }
                    }
                    if (colHasWall) {
                        hitWall = true;
                        doorCol = x;
                    }
                    if (!colHasWall && hitWall) break;
                    for (int y = 0; y < currentRoomSize; ++y) {
                        if (std::abs((y + offset) - roomCenter) <= Constants::CORRIDOR_WIDTH / 2) {
                            clearTile(startY + offset + y, startX + offset + x);
                        }
                    }
                }
                int halfH2 = Constants::CORRIDOR_WIDTH / 2;
                if (doorCol >= 0) {
                    for (int dy = -halfH2; dy <= halfH2; ++dy) {
                        baked->layer1_objects[startY + roomCenter + dy][startX + offset + doorCol] = 20;
                    }
                } else {
                    for (int dy = -halfH2; dy <= halfH2; ++dy) {
                        baked->layer1_objects[startY + roomCenter + dy][startX + offset + currentRoomSize - 1] = 20;
                    }
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
            for (int y = roomCenter - Constants::CORRIDOR_WIDTH / 2 - 1; y <= roomCenter + Constants::CORRIDOR_WIDTH / 2 + 1; ++y) {
                for (int x = offset + currentRoomSize; x < roomOuterSize; ++x) {
                    int px = startX + x;
                    int py = startY + y;
                    bool isWall = (y == roomCenter - Constants::CORRIDOR_WIDTH / 2 - 1 || y == roomCenter + Constants::CORRIDOR_WIDTH / 2 + 1);
                    baked->layer0_tiles[py][px] = isWall ? 1 : 0;
                }
            }
        }
        
        if (node->south) {
            for (int x = roomCenter - Constants::CORRIDOR_WIDTH / 2 - 1; x <= roomCenter + Constants::CORRIDOR_WIDTH / 2 + 1; ++x) {
                for (int y = offset + currentRoomSize; y < roomOuterSize; ++y) {
                    int px = startX + x;
                    int py = startY + y;
                    bool isWall = (x == roomCenter - Constants::CORRIDOR_WIDTH / 2 - 1 || x == roomCenter + Constants::CORRIDOR_WIDTH / 2 + 1);
                    baked->layer0_tiles[py][px] = isWall ? 1 : 0;
                }
            }
        }
        
        if (node->west) {
            for (int y = roomCenter - Constants::CORRIDOR_WIDTH / 2 - 1; y <= roomCenter + Constants::CORRIDOR_WIDTH / 2 + 1; ++y) {
                for (int x = 0; x < offset; ++x) {
                    int px = startX + x;
                    int py = startY + y;
                    bool isWall = (y == roomCenter - Constants::CORRIDOR_WIDTH / 2 - 1 || y == roomCenter + Constants::CORRIDOR_WIDTH / 2 + 1);
                    baked->layer0_tiles[py][px] = isWall ? 1 : 0;
                }
            }
        }
        
        if (node->north) {
            for (int x = roomCenter - Constants::CORRIDOR_WIDTH / 2 - 1; x <= roomCenter + Constants::CORRIDOR_WIDTH / 2 + 1; ++x) {
                for (int y = 0; y < offset; ++y) {
                    int px = startX + x;
                    int py = startY + y;
                    bool isWall = (x == roomCenter - Constants::CORRIDOR_WIDTH / 2 - 1 || x == roomCenter + Constants::CORRIDOR_WIDTH / 2 + 1);
                    baked->layer0_tiles[py][px] = isWall ? 1 : 0;
                }
            }
        }
    }
    
    return baked;
}

void TilemapRenderer::DrawRoomBase(const RoomTemplate& room, Vector2 roomOffsetWorldPos, Texture2D floorTileset, Texture2D wallTileset, Texture2D prop1Texture, Texture2D prop2Texture, Texture2D boxTexture) {
    float scaledTileSize = Constants::RENDER_TILE_SIZE;

    Rectangle wallFrontFaceSrc[2] = { {0.1f, 16.1f, 15.8f, 15.8f}, {16.1f, 16.1f, 15.8f, 15.8f} };
    
    Rectangle floorSrc[6];
    for(int i = 0; i < 6; ++i) {
        floorSrc[i] = { (float)(i * 16) + 0.1f, 0.1f, 15.8f, 15.8f };
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
            
            if (tileType == 2 || tileType == 1) continue; 
            
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

    // Draw Wall Front Faces
    for (int y = 0; y < room.height; ++y) {
        for (int x = 0; x < room.width; ++x) {
            int tileType = room.layer0_tiles[y][x];
            int objType = room.layer1_objects[y][x];
            
            if (tileType == 1) {
                int variant = std::abs(hash(x, y)) % 2;
                bool tileBelowIsFloorOrVoid = false;
                if (y + 1 >= room.height) {
                    tileBelowIsFloorOrVoid = true;
                } else {
                    int typeBelow = room.layer0_tiles[y+1][x];
                    if (typeBelow == 0 || typeBelow == 2) {
                        tileBelowIsFloorOrVoid = true;
                    }
                }
                
                if (tileBelowIsFloorOrVoid) {
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

void TilemapRenderer::GetRoomDepthRenderItems(const RoomTemplate& room, Vector2 roomOffsetWorldPos, Texture2D wallTileset, Texture2D prop1Texture, Texture2D prop2Texture, Texture2D boxTexture, std::vector<DepthRenderItem>& items) {
    float scaledTileSize = Constants::RENDER_TILE_SIZE;
    Rectangle wallTopSrc[2] = { {0.1f, 0.1f, 15.8f, 15.8f}, {16.1f, 0.1f, 15.8f, 15.8f} };

    auto hash = [](int x, int y) -> int {
        unsigned int h = (unsigned int)(x * 374761393 ^ y * 668265263);
        h = (h ^ (h >> 13)) * 1274126177;
        return h ^ (h >> 16);
    };

    for (int y = 0; y < room.height; ++y) {
        for (int x = 0; x < room.width; ++x) {
            int tileType = room.layer0_tiles[y][x];
            int objType = room.layer1_objects[y][x];
            
            if (tileType == 1) {
                float ySort = std::floor(roomOffsetWorldPos.y + (y + 1) * scaledTileSize);
                items.push_back({
                    ySort,
                    [x, y, roomOffsetWorldPos, scaledTileSize, wallTileset, wallTopSrc, hash]() {
                        Rectangle destRec = {
                            std::floor(roomOffsetWorldPos.x + x * scaledTileSize),
                            std::floor(roomOffsetWorldPos.y + y * scaledTileSize),
                            scaledTileSize,
                            scaledTileSize
                        };
                        int variant = std::abs(hash(x, y)) % 2;
                        DrawTexturePro(wallTileset, wallTopSrc[variant], destRec, {0,0}, 0.0f, WHITE);
                    }
                });
            }
        }
    }
}
