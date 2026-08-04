#include "Core/Manager/LevelManager.h"
#include "Core/Constants.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Core/EntityFactory.h"
#include "Entities/Enemy.h"
#include "Entities/Items/DestructibleBox.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <utility>

    constexpr float COLLISION_EDGE_PADDING = 0.001f;
    constexpr float PARTIAL_WALL_WIDTH = 9.0f;
    constexpr float RIGHT_PARTIAL_WALL_OFFSET = Constants::RENDER_TILE_SIZE - PARTIAL_WALL_WIDTH;
    constexpr int DRAW_PADDING_TILES = 20;

    bool LoadCsvGrid(
        const std::string& filepath,
        std::vector<std::vector<int>>& output
    ) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open level layer: " << filepath
                      << std::endl;
            return false;
        }

        output.clear();
        std::string line;
        std::size_t expectedColumns = 0;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            std::stringstream rowStream(line);
            std::string cellString;
            std::vector<int> row;
            while (std::getline(rowStream, cellString, ',')) {
                try {
                    row.push_back(std::stoi(cellString));
                } catch (...) {
                    std::cerr << "Invalid CSV value in " << filepath
                              << std::endl;
                    output.clear();
                    return false;
                }
            }

            if (row.empty()) {
                std::cerr << "Empty CSV row in " << filepath << std::endl;
                output.clear();
                return false;
            }
            if (expectedColumns == 0) {
                expectedColumns = row.size();
            } else if (row.size() != expectedColumns) {
                std::cerr << "Non-rectangular CSV layer: " << filepath
                          << std::endl;
                output.clear();
                return false;
            }
            output.push_back(std::move(row));
        }

        return !output.empty();
    }

LevelManager::LevelManager()
    : levelWidth(0.0f), levelHeight(0.0f), gridRows(0), gridCols(0) {
    tileset = LoadTexture("assets/tileset/Galra_ship_Tileset.png");
    floorTileset = LoadTexture("assets/tileset/Galra_Floors.png");
    wallTileset = LoadTexture("assets/tileset/Galra_Walls.png");
    boxTexture = LoadTexture("assets/Objects/box.png");
    gateTexture = LoadTexture("assets/Objects/Transfer_gate.png");
    
    SetTextureFilter(tileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(floorTileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(wallTileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(boxTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(gateTexture, TEXTURE_FILTER_POINT);
}

LevelManager::~LevelManager() {
    ClearLevel();
    UnloadTexture(tileset);
    UnloadTexture(floorTileset);
    UnloadTexture(wallTileset);
    UnloadTexture(boxTexture);
    UnloadTexture(gateTexture);
}

bool LevelManager::LoadObjectGrid(const std::string& filepath) {
    mapObjectGrid.clear();
    mapObjectGrid.reserve(mapGridLayer1.size());
    for (const auto& row : mapGridLayer1) {
        mapObjectGrid.push_back(
            std::vector<MapObjectId>(row.size(), MapObjectId::Empty)
        );
    }

    std::string objectLayerPath = filepath;
    size_t layerNamePosition = objectLayerPath.find("Layer 1");
    if (layerNamePosition == std::string::npos) {
        return false;
    }

    objectLayerPath.replace(
        layerNamePosition,
        std::string("Layer 1").size(),
        "Game Objects"
    );

    std::ifstream objectFile(objectLayerPath);
    if (!objectFile.is_open()) {
        return false;
    }

    bool dimensionsMatch = true;
    std::string line;
    int rowIndex = 0;
    while (std::getline(objectFile, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (rowIndex >= (int)mapObjectGrid.size()) {
            dimensionsMatch = false;
            rowIndex++;
            continue;
        }

        std::stringstream rowStream(line);
        std::string cellString;
        int columnIndex = 0;
        while (std::getline(rowStream, cellString, ',')) {
            if (columnIndex >= (int)mapObjectGrid[rowIndex].size()) {
                dimensionsMatch = false;
                columnIndex++;
                continue;
            }

            try {
                mapObjectGrid[rowIndex][columnIndex] =
                    static_cast<MapObjectId>(std::stoi(cellString));
            } catch (...) {
                mapObjectGrid[rowIndex][columnIndex] = MapObjectId::Empty;
            }
            columnIndex++;
        }

        if (columnIndex != (int)mapObjectGrid[rowIndex].size()) {
            dimensionsMatch = false;
        }
        rowIndex++;
    }

    if (rowIndex != (int)mapObjectGrid.size()) {
        dimensionsMatch = false;
    }

    if (!dimensionsMatch) {
        std::cerr
            << "Game Objects layer dimensions do not match Layer 1: "
            << objectLayerPath
            << std::endl;
    }

    return dimensionsMatch;
}

void LevelManager::SpawnGameObjects(TeamManager* teamManager) {
    for (int row = 0; row < (int)mapObjectGrid.size(); ++row) {
        for (int column = 0;
             column < (int)mapObjectGrid[row].size();
             ++column) {
            MapObjectId objectId = mapObjectGrid[row][column];
            if (objectId == MapObjectId::Empty) {
                continue;
            }

            AddEntity(EntityFactory::CreateEntity(
                objectId,
                TileToWorld(column, row),
                { row, column },
                teamManager,
                GetLevelAccessBundle()
            ));
        }
    }
}

void LevelManager::LoadLevel(const std::string& filepath, TeamManager* teamManager) {
    ClearLevel();
    levelMode = LevelMode::Layered;

    if (filepath.find("Layer 1") == std::string::npos ||
        filepath.size() < 4 ||
        filepath.substr(filepath.size() - 4) != ".csv") {
        std::cerr << "Level path must reference a Layer 1 CSV: " << filepath << std::endl;
        return;
    }

    if (!LoadCsvGrid(filepath, mapGridLayer1)) {
        return;
    }

    std::string layer2Path = filepath;
    layer2Path.replace(layer2Path.find("Layer 1"), 7, "Layer 2");
    if (!LoadCsvGrid(layer2Path, mapGridLayer2)) {
        ClearLevel();
        return;
    }

    if (mapGridLayer2.size() != mapGridLayer1.size() ||
        mapGridLayer2.front().size() != mapGridLayer1.front().size()) {
        std::cerr << "Layer 2 dimensions do not match Layer 1: " << layer2Path << std::endl;
        ClearLevel();
        return;
    }

    if (!LoadObjectGrid(filepath)) {
        ClearLevel();
        return;
    }

    gridRows = static_cast<int>(mapGridLayer1.size());
    gridCols = static_cast<int>(mapGridLayer1.front().size());
    levelWidth = gridCols * Constants::RENDER_TILE_SIZE;
    levelHeight = gridRows * Constants::RENDER_TILE_SIZE;

    // Spawn NPC for dialogue interaction
    Vector2 npcPos = {
        10 * Constants::RENDER_TILE_SIZE + Constants::RENDER_TILE_SIZE / 2.0f,
        10 * Constants::RENDER_TILE_SIZE + Constants::RENDER_TILE_SIZE / 2.0f
    };
    GameObject* npc = EntityFactory::CreateEntity(
        MapObjectId::NPC,
        npcPos,
        {0, 0},
        teamManager,
        GetLevelAccessBundle()
    );
    if (npc) {
        AddEntity(npc);
    }
    
    // Spawn objects
    SpawnGameObjects(teamManager);

    // Store in GameManager for global access
    GameManager::GetInstance().SetLevelBounds(levelWidth, levelHeight);
}

void LevelManager::UpdateLevel(float deltaTime, Vector2 playerPos) {
    if (IsProceduralDungeon()) {
        if (currentlyLockedRoom && currentlyLockedRoom->state == RoomState::CLEARED) {
            doorColliders.clear();
            currentlyLockedRoom = nullptr;
        }

        if (!currentlyLockedRoom) {
            // Build a small collision rect from the player position
            Rectangle playerBox = { playerPos.x - 8, playerPos.y - 8, 16, 16 };

            if (playerBox.width > 0) {
                for (auto& node : levelMap.generatedNodes) {
                    bool playerInside = CheckCollisionRecs(playerBox, node->triggerBounds);

                    if (playerInside) {
                        node->isDiscovered = true;
                    }

                    if (playerInside && (node->type == RoomType::BATTLE || node->type == RoomType::BOSS) && node->state == RoomState::IDLE) {
                        node->state = RoomState::LOCKED;
                        currentlyLockedRoom = node;

                        // Generate dynamic doors blocking exits
                        doorColliders.clear();
                        float tileW = Constants::RENDER_TILE_SIZE;
                        int roomOuterSize = Constants::MAX_ROOM_TILE_SIZE + Constants::CORRIDOR_LENGTH;
                        int startX = node->gridX * roomOuterSize;
                        int startY = node->gridY * roomOuterSize;
                        
                        int currentRoomSize = (node->type == RoomType::BATTLE || node->type == RoomType::BOSS) 
                                              ? Constants::MAX_ROOM_TILE_SIZE 
                                              : Constants::NORMAL_ROOM_TILE_SIZE;
                        int offset = (Constants::MAX_ROOM_TILE_SIZE - currentRoomSize) / 2;
                        
                        for (int y = 0; y < currentRoomSize; ++y) {
                            for (int x = 0; x < currentRoomSize; ++x) {
                                if (activeRoom->layer1_objects[startY + offset + y][startX + offset + x] == 20) {
                                    doorColliders.push_back({
                                        (startX + offset + x) * tileW,
                                        (startY + offset + y) * tileW,
                                        tileW,
                                        tileW
                                    });
                                }
                            }
                        }

                        // Nudge player to room center so they don't get stuck inside a door collider
                        float roomCenterX = (startX + Constants::MAX_ROOM_TILE_SIZE / 2.0f) * tileW;
                        float roomCenterY = (startY + Constants::MAX_ROOM_TILE_SIZE / 2.0f) * tileW;
                        // Move toward center, but only if currently overlapping a door
                        Rectangle pBox = playerBox;
                        bool overlappingDoor = false;
                        for (const auto& door : doorColliders) {
                            if (CheckCollisionRecs(pBox, door)) {
                                overlappingDoor = true;
                                break;
                            }
                        }
                        if (overlappingDoor) {
                            // Store the nudge position for the caller to apply
                            nudgePosition = { roomCenterX, roomCenterY };
                            needsNudge = true;
                        }

                        break;
                    }
                }
            }
        }
    }

    // Always update entities!
    enemyPathManager.Update(*this, deltaTime);

    for (auto it = levelEntities.begin(); it != levelEntities.end(); it++) {
        if ((*it)->GetObjectType() == GameObjectType::Enemy) {
            Enemy* e = static_cast<Enemy*>(*it);
            if (e->IsDead()) {
                continue;
            }
        }

        (*it)->Update(deltaTime);
    }

    ProcessPendingMapObjectDestructions();
    ProcessPendingRemovals();
}

void LevelManager::DrawLevel() {
    if (IsProceduralDungeon()) {
        if (activeRoom) {
            TilemapRenderer::DrawRoom(*activeRoom, roomOffset, floorTileset, wallTileset);
            
            // Draw EXIT gate if this room is an EXIT room
            for (const auto& node : levelMap.generatedNodes) {
                if (node->type == RoomType::EXIT) {
                    float tileW = Constants::RENDER_TILE_SIZE;
                    int roomOuterSize = Constants::MAX_ROOM_TILE_SIZE + Constants::CORRIDOR_LENGTH;
                    float startX = node->gridX * roomOuterSize * tileW;
                    float startY = node->gridY * roomOuterSize * tileW;
                    
                    float roomCenterX = startX + (Constants::MAX_ROOM_TILE_SIZE / 2.0f) * tileW;
                    float roomCenterY = startY + (Constants::MAX_ROOM_TILE_SIZE / 2.0f) * tileW;
                    
                    Rectangle destRec = {
                        roomCenterX - tileW * 2.0f,
                        roomCenterY - tileW * 2.0f,
                        tileW * 4.0f,
                        tileW * 4.0f
                    };
                    
                    float frameWidth = gateTexture.width / 8.0f;
                    int currentFrame = (int)(GetTime() * 10) % 8;
                    Rectangle gateFrameSrc = {currentFrame * frameWidth, 0, frameWidth, (float)gateTexture.height};
                    DrawTexturePro(gateTexture, gateFrameSrc, destRec, {0,0}, 0.0f, WHITE);
                }
            }
        }
    } else {
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

        for (int layer = 1; layer <= 2; ++layer) {
            const auto& currentGrid = (layer == 1) ? mapGridLayer1 : mapGridLayer2;
            if (currentGrid.empty()) continue;

            // Draw Floors pass
            for (int r = -DRAW_PADDING_TILES; r < gridRows + DRAW_PADDING_TILES; ++r) {
                for (int c = -DRAW_PADDING_TILES; c < gridCols + DRAW_PADDING_TILES; ++c) {
                    if (r >= 0 && c >= 0 && r < gridRows && c < currentGrid[r].size()) {
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
                            int variant = std::abs(hash(c, r)) % 6;
                            DrawTexturePro(floorTileset, floorSrc[variant], destRec, {0,0}, 0.0f, WHITE);
                        }
                    }
                }
            }
            
            // Draw Walls pass
            for (int r = -DRAW_PADDING_TILES; r < gridRows + DRAW_PADDING_TILES; ++r) {
                for (int c = -DRAW_PADDING_TILES; c < gridCols + DRAW_PADDING_TILES; ++c) {
                    bool isWall = false;
                    if (r >= 0 && c >= 0 && r < gridRows && c < currentGrid[r].size()) {
                        int tid = currentGrid[r][c];
                        // Wall tile IDs: 0 (void/border) and 4-11 (solid walls)
                        if (tid == 0 || (tid >= 4 && tid <= 11)) isWall = true;
                    }
                    
                    if (isWall) {
                        Rectangle destRec = {
                            std::floor((float)c * Constants::RENDER_TILE_SIZE),
                            std::floor((float)r * Constants::RENDER_TILE_SIZE),
                            Constants::RENDER_TILE_SIZE,
                            Constants::RENDER_TILE_SIZE
                        };
                        int variant = std::abs(hash(c, r)) % 2;
                        DrawTexturePro(wallTileset, wallTopSrc[variant], destRec, {0,0}, 0.0f, WHITE);
                        
                        // Depth logic for legacy map
                        bool tileBelowIsFloor = true; // Always draw fronts for legacy map border walls
                        if (r + 1 >= 0 && r + 1 < gridRows && c >= 0 && c < currentGrid[r+1].size()) {
                            int belowTid = currentGrid[r+1][c];
                            if (belowTid == 0 || (belowTid >= 4 && belowTid <= 11)) tileBelowIsFloor = false; // Only hide front if the tile immediately below is another wall
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
    } // End legacy map check

    for (auto* entity : levelEntities) {
        entity->Draw();
    }

    if (Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES ||
        Constants::DEBUG_DRAW_ENEMY_PATHS) {
        for (GameObject* entity : levelEntities) {
            if (entity->GetObjectType() == GameObjectType::Enemy) {
                static_cast<Enemy*>(entity)->DrawPathDebug();
            }
        }
    }
}

void LevelManager::ClearLevel() {
    pendingMapObjectDestructions.clear();
    pendingRemoval.clear();
    enemyPathManager.Clear();

    for (auto* entity : levelEntities) {
        delete entity;
    }
    levelEntities.clear();
    mapGridLayer1.clear();
    mapGridLayer2.clear();
    mapObjectGrid.clear();
    activeRoom = nullptr;
    currentRoomWalls.clear();
    doorColliders.clear();
    currentlyLockedRoom = nullptr;
    roomOffset = {0.0f, 0.0f};
    nudgePosition = {0.0f, 0.0f};
    needsNudge = false;
    levelMode = LevelMode::Layered;
}

void LevelManager::AddEntity(GameObject* entity) {
    if (entity) {
        levelEntities.push_back(entity);
    }
}

bool LevelManager::IsSolidCollision(Rectangle box) const {
    if (IsProceduralDungeon()) {
        for (const auto& wallRect : currentRoomWalls) {
            if (CheckCollisionRecs(box, wallRect)) {
                return true;
            }
        }
        for (const auto& doorRect : doorColliders) {
            if (CheckCollisionRecs(box, doorRect)) {
                return true;
            }
        }
        return false;
    }

    // Find min and max tile indices that overlap with the box
    // Box might have negative coords if outside, so use floor
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
                if (r < mapGridLayer1.size() && c < mapGridLayer1[r].size()) tileID1 = mapGridLayer1[r][c];
                if (r < mapGridLayer2.size() && c < mapGridLayer2[r].size()) tileID2 = mapGridLayer2[r][c];
                if (r < mapObjectGrid.size() && c < mapObjectGrid[r].size()) {
                    mapObjectId = mapObjectGrid[r][c];
                }
            } else {
                return true; // Out of bounds is fully solid void
            }

            int tileIDs[] = {tileID1, tileID2};
            for (int tileID : tileIDs) {
                if (tileID == 0) {
                    return true; // Void is solid boundary
                } else if (tileID >= 4 && tileID <= 11) {
                    return true; // Fully solid
                } else if (tileID >= 1 && tileID <= 3) {
                    // Right 9px solid
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
                    // Left 9px solid
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

            if (IsSolidMapObject(mapObjectId)) {
                return true;
            }
        }
    }
    return false;
}

bool LevelManager::IsSolidMapObject(MapObjectId objectId) const {
    if (objectId == MapObjectId::DestructibleBox) {
        return true; // DestructibleBox occupies one solid cell.
    }

    return false;
}

bool LevelManager::HasClearLineOfSight(
    Vector2 start,
    Vector2 end,
    float projectileRadius
) const {
    constexpr float MAX_PROBE_SPACING = 8.0f;

    Vector2 segment = {
        end.x - start.x,
        end.y - start.y
    };
    float distance = std::sqrt(segment.x * segment.x + segment.y * segment.y);
    float radius = std::max(projectileRadius, COLLISION_EDGE_PADDING);
    if (distance <= COLLISION_EDGE_PADDING) {
        Rectangle probe = {
            start.x - radius,
            start.y - radius,
            radius * 2.0f,
            radius * 2.0f
        };
        return !IsSolidCollision(probe);
    }

    int probeCount = std::max(1, (int)std::ceil(distance / MAX_PROBE_SPACING));

    // Entities are not part of IsSolidCollision, so both endpoints can be
    // checked safely. This catches a shooter, muzzle, or target overlapping
    // actual level geometry.
    for (int probeIndex = 0; probeIndex <= probeCount; ++probeIndex) {
        float amount = (float)probeIndex / (float)probeCount;
        Vector2 point = {
            start.x + segment.x * amount,
            start.y + segment.y * amount
        };
        Rectangle probe = {
            point.x - radius,
            point.y - radius,
            radius * 2.0f,
            radius * 2.0f
        };

        if (IsSolidCollision(probe)) {
            return false;
        }
    }

    return true;
}

bool LevelManager::IsValidSpawnLocation(const GameObject* entity) const {
    if (!entity) return false;

    if (entity->GetObjectType() == GameObjectType::Enemy) {
        const Enemy* enemy = static_cast<const Enemy*>(entity);
        return !IsSolidCollision(enemy->GetNavigationFootprintAt(
            enemy->GetPosition()
        ));
    }

    return !IsSolidCollision(entity->GetBoundingBox());
}

Vector2 LevelManager::WorldToTile(Vector2 worldPos) const {
    return {
        std::floor(worldPos.x / Constants::RENDER_TILE_SIZE),
        std::floor(worldPos.y / Constants::RENDER_TILE_SIZE)
    };
}

Vector2 LevelManager::TileToWorld(int tileX, int tileY) const {
    return {
        tileX * Constants::RENDER_TILE_SIZE + Constants::RENDER_TILE_SIZE / 2.0f,
        tileY * Constants::RENDER_TILE_SIZE + Constants::RENDER_TILE_SIZE / 2.0f
    };
}

//////////////////////////////////////////////
// Narrow level-access capabilities used by entities.
//////////////////////////////////////////////

void LevelManager::ProcessPendingMapObjectDestructions() {
    for (const PendingMapObjectDestruction& request
         : pendingMapObjectDestructions) {
        GameObject* object = request.object;
        if (!object) {
            continue;
        }

        if (std::find(levelEntities.begin(), levelEntities.end(), object)
            == levelEntities.end()) {
            continue;
        }

        if (object->GetObjectType() != GameObjectType::Box) {
            continue;
        }

        int row = request.cell.row;
        int column = request.cell.column;
        if (row < 0 || column < 0 ||
            row >= (int)mapObjectGrid.size() ||
            column >= (int)mapObjectGrid[row].size()) {
            continue;
        }

        if (mapObjectGrid[row][column] != MapObjectId::DestructibleBox) {
            continue;
        }

        mapObjectGrid[row][column] = MapObjectId::Empty;
        QueueRemoval(object);
    }

    pendingMapObjectDestructions.clear();
}

void LevelManager::ProcessPendingRemovals() {
    for (GameObject* entity : pendingRemoval) {
        auto it = std::find(levelEntities.begin(), levelEntities.end(), entity);
        if (it != levelEntities.end()) {
            delete *it;
            levelEntities.erase(it);
        }
    }
    pendingRemoval.clear();
}

void LevelManager::BeginPathFinding(Enemy& enemy) {
    enemyPathManager.AddEnemy(enemy);
}

void LevelManager::EndPathFinding(Enemy& enemy) {
    enemyPathManager.RemoveEnemy(enemy);
}

bool LevelManager::IsBlocked(Rectangle bounds) const {
    return IsSolidCollision(bounds);
}

Rectangle LevelManager::GetLevelBounds() const {
    return { 0.0f, 0.0f, levelWidth, levelHeight };
}

Rectangle LevelManager::GetCurrentRoomBounds() const {
    if (IsProceduralDungeon() && currentlyLockedRoom && currentlyLockedRoom->state == RoomState::LOCKED) {
        float tileW = Constants::RENDER_TILE_SIZE;
        int roomOuterSize = Constants::MAX_ROOM_TILE_SIZE + Constants::CORRIDOR_LENGTH;
        
        int currentRoomSize = (currentlyLockedRoom->type == RoomType::BATTLE || currentlyLockedRoom->type == RoomType::BOSS) 
                              ? Constants::MAX_ROOM_TILE_SIZE 
                              : Constants::NORMAL_ROOM_TILE_SIZE;
        int offset = (Constants::MAX_ROOM_TILE_SIZE - currentRoomSize) / 2;
                              
        float startX = (currentlyLockedRoom->gridX * roomOuterSize + offset) * tileW;
        float startY = (currentlyLockedRoom->gridY * roomOuterSize + offset) * tileW;
        float roomW = currentRoomSize * tileW;
        float roomH = currentRoomSize * tileW;
        
        return { startX + tileW, startY + tileW, roomW - 2 * tileW, roomH - 2 * tileW };
    }
    return { 0.0f, 0.0f, levelWidth, levelHeight };
}

bool LevelManager::IsPlayerInExitRoom(Vector2 playerPos) const {
    if (!IsProceduralDungeon()) return false;
    
    Rectangle playerBox = { playerPos.x - 8, playerPos.y - 8, 16, 16 };
    for (const auto& node : levelMap.generatedNodes) {
        if (node->type == RoomType::EXIT) {
            float tileW = Constants::RENDER_TILE_SIZE;
            int roomOuterSize = Constants::MAX_ROOM_TILE_SIZE + Constants::CORRIDOR_LENGTH;
            float startX = node->gridX * roomOuterSize * tileW;
            float startY = node->gridY * roomOuterSize * tileW;
            
            // Define the center interactable blue rectangle
            float roomCenterX = startX + (Constants::MAX_ROOM_TILE_SIZE / 2.0f) * tileW;
            float roomCenterY = startY + (Constants::MAX_ROOM_TILE_SIZE / 2.0f) * tileW;
            Rectangle exitGate = { roomCenterX - 32, roomCenterY - 32, 64, 64 };
            
            if (CheckCollisionRecs(playerBox, exitGate)) {
                return true;
            }
        }
    }
    return false;
}

std::optional<Vector2> LevelManager::GetNextMoveTarget(
    Enemy& enemy
) {
    return enemyPathManager.GetNextMoveTarget(
        *this,
        enemy
    );
}

Vector2 LevelManager::GetLocalDirection(
    Enemy& enemy,
    Vector2 desiredDirection
) {
    return enemyPathManager.GetLocalAvoidanceDirection(
        *this,
        enemy,
        desiredDirection
    );
}

void LevelManager::QueueRemoval(GameObject* entity) {
    if (!entity) return;

    if (std::find(levelEntities.begin(), levelEntities.end(), entity) == levelEntities.end()) {
        return;
    }

    if (std::find(pendingRemoval.begin(), pendingRemoval.end(), entity) == pendingRemoval.end()) {
        pendingRemoval.push_back(entity);
    }
}

void LevelManager::QueueMapObjectDestruction(
    GameObject& object,
    GameObjectCell cell
) {
    if (object.GetObjectType() != GameObjectType::Box) {
        return;
    }

    if (std::find(levelEntities.begin(), levelEntities.end(), &object)
        == levelEntities.end()) {
        return;
    }

    if (cell.row < 0 || cell.column < 0 ||
        cell.row >= (int)mapObjectGrid.size() ||
        cell.column >= (int)mapObjectGrid[cell.row].size()) {
        return;
    }

    if (mapObjectGrid[cell.row][cell.column]
        != MapObjectId::DestructibleBox) {
        return;
    }

    auto duplicate = std::find_if(
        pendingMapObjectDestructions.begin(),
        pendingMapObjectDestructions.end(),
        [&object](const PendingMapObjectDestruction& request) {
            return request.object == &object;
        }
    );
    if (duplicate == pendingMapObjectDestructions.end()) {
        pendingMapObjectDestructions.push_back({ &object, cell });
    }
}

void LevelManager::GenerateDungeon(TeamManager* teamManager) {
    ClearLevel();
    levelMode = LevelMode::Procedural;
    currentlyLockedRoom = nullptr;

    levelMap.Generate(7, 7);
    activeRoom = levelMap.BakeLevel();
    roomOffset = {0.0f, 0.0f};
    
    currentRoomWalls = activeRoom->GenerateWallColliders(roomOffset, Constants::RENDER_TILE_SIZE, 1.0f);
    
    levelWidth = activeRoom->width * Constants::RENDER_TILE_SIZE;
    levelHeight = activeRoom->height * Constants::RENDER_TILE_SIZE;
    GameManager::GetInstance().SetLevelBounds(levelWidth, levelHeight);

    // Teleport player to spawn room center
    if (levelMap.spawnRoom) {
        float tileW = Constants::RENDER_TILE_SIZE;
        int roomOuterSize = Constants::MAX_ROOM_TILE_SIZE + Constants::CORRIDOR_LENGTH;
        float spawnWorldX = (levelMap.spawnRoom->gridX * roomOuterSize + Constants::MAX_ROOM_TILE_SIZE / 2.0f) * tileW;
        float spawnWorldY = (levelMap.spawnRoom->gridY * roomOuterSize + Constants::MAX_ROOM_TILE_SIZE / 2.0f) * tileW;
        teamManager->GetActivePaladin()->SetPosition({spawnWorldX, spawnWorldY});

        // Auto-discover spawn room and mark it cleared (no combat in spawn)
        levelMap.spawnRoom->isDiscovered = true;
        levelMap.spawnRoom->state = RoomState::CLEARED;
    }
}
