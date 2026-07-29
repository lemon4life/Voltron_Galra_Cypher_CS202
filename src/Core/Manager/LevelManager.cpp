#include "Core/Manager/LevelManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/EntityFactory.h"

#include "Entities/Enemy.h"
#include "Entities/Items/DestructibleBox.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace {
    constexpr int TILE_SIZE_PIXELS = 32;
    constexpr float TILE_SIZE = 32.0f;
    constexpr float COLLISION_EDGE_PADDING = 0.001f;
    constexpr float PARTIAL_WALL_WIDTH = 9.0f;
    constexpr float RIGHT_PARTIAL_WALL_OFFSET = TILE_SIZE - PARTIAL_WALL_WIDTH;
    constexpr int DRAW_PADDING_TILES = 20;
}

LevelManager::LevelManager()
    : levelWidth(0.0f), levelHeight(0.0f), gridRows(0), gridCols(0) {
    tileset = LoadTexture("assets/tileset/Galra_ship_Tileset.png");
    SetTextureFilter(tileset, TEXTURE_FILTER_POINT);
}

LevelManager::~LevelManager() {
    ClearLevel();
    UnloadTexture(tileset);
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

    return true;
}

void LevelManager::SpawnGameObjects() {
    for (int row = 0; row < (int)mapObjectGrid.size(); ++row) {
        for (int column = 0;
             column < (int)mapObjectGrid[row].size();
             ++column) {
            if (mapObjectGrid[row][column] != MapObjectId::DestructibleBox) {
                continue;
            }

            AddEntity(new DestructibleBox(
                TileToWorld(column, row),
                { row, column },
                *this
            ));
        }
    }
}

void LevelManager::LoadLevel(const std::string& filepath, TeamManager* teamManager) {
    ClearLevel();

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open level file: " << filepath << std::endl;
        return;
    }

    std::string line;
    mapGridLayer1.clear();
    mapGridLayer2.clear();
    mapObjectGrid.clear();
    gridCols = 0;

    bool isCSV = (filepath.length() >= 4 && filepath.substr(filepath.length() - 4) == ".csv");

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back(); // Handle Windows line endings
        }

        std::vector<int> row;
        if (isCSV) {
            std::stringstream ss(line);
            std::string cellString;
            while (std::getline(ss, cellString, ',')) {
                try {
                    row.push_back(std::stoi(cellString));
                } catch (const std::invalid_argument& e) {
                    row.push_back(-1); // -1 is empty in 0-based tiled CSV
                }
            }
        } else {
            // Legacy .txt map parsing
            for (int c = 0; c < line.length(); ++c) {
                char type = line[c];
                int tileID = -1;

                if (type == 'W') tileID = 5; // Map to Wall Face ID
                else if (type == '.') tileID = 20; // Map to Floor ID
                else if (type == 'N' || type == 'E' || type == 'R' ||
                         type == 'D' || type == 'B') {
                    tileID = 20; // Map to Floor ID
                    Vector2 tileCenter = {
                        (float)c * TILE_SIZE + TILE_SIZE / 2.0f,
                        (float)mapGridLayer1.size() * TILE_SIZE + TILE_SIZE / 2.0f
                    };

                    GameObject* entity = EntityFactory::CreateEntity(
                        type,
                        tileCenter,
                        teamManager,
                        GetLevelAccessBundle()
                    );
                    if (entity) {
                        AddEntity(entity);
                    }
                }
                row.push_back(tileID);
            }
        }

        if (row.size() > gridCols) gridCols = row.size();
        mapGridLayer1.push_back(row);
    }
    file.close();

    // If this is a CSV map, attempt to load Layer 2
    if (isCSV) {
        // Initialize mapGridLayer2 with -1s
        for (int i = 0; i < mapGridLayer1.size(); ++i) {
            mapGridLayer2.push_back(std::vector<int>(mapGridLayer1[i].size(), -1));
        }

        std::string layer2Path = filepath;
        size_t pos = layer2Path.find("Layer 1");
        if (pos != std::string::npos) {
            layer2Path.replace(pos, 7, "Layer 2");
            std::ifstream file2(layer2Path);
            if (file2.is_open()) {
                int r = 0;
                while (std::getline(file2, line) && r < mapGridLayer2.size()) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    std::stringstream ss(line);
                    std::string cellString;
                    int c = 0;
                    while (std::getline(ss, cellString, ',') && c < mapGridLayer2[r].size()) {
                        try {
                            int id2 = std::stoi(cellString);
                            mapGridLayer2[r][c] = id2;
                        } catch (...) {}
                        c++;
                    }
                    r++;
                }
                file2.close();
            }
        }
    }

    gridRows = mapGridLayer1.size();
    levelWidth = gridCols * TILE_SIZE;
    levelHeight = gridRows * TILE_SIZE;

    if (isCSV) {
        LoadObjectGrid(filepath);
    } else {
        mapObjectGrid.reserve(mapGridLayer1.size());
        for (const auto& row : mapGridLayer1) {
            mapObjectGrid.push_back(
                std::vector<MapObjectId>(row.size(), MapObjectId::Empty)
            );
        }
    }

    SpawnGameObjects();

    // Store in GameManager for global access
    GameManager::GetInstance().SetLevelBounds(levelWidth, levelHeight);
}

void LevelManager::UpdateLevel(float deltaTime) {
    ProcessPendingMapObjectDestructions();
    ProcessPendingRemovals();

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
    int tilesetCols = tileset.width / TILE_SIZE_PIXELS;
    if (tilesetCols <= 0) tilesetCols = 1; // Fallback to avoid division by zero

    for (int layer = 1; layer <= 2; ++layer) {
        const auto& currentGrid = (layer == 1) ? mapGridLayer1 : mapGridLayer2;
        if (currentGrid.empty()) continue;

        for (int r = -DRAW_PADDING_TILES; r < gridRows + DRAW_PADDING_TILES; ++r) {
            for (int c = -DRAW_PADDING_TILES; c < gridCols + DRAW_PADDING_TILES; ++c) {
                Rectangle destRec = {
                    std::floor((float)c * TILE_SIZE),
                    std::floor((float)r * TILE_SIZE),
                    TILE_SIZE,
                    TILE_SIZE
                };
                Rectangle sourceRec = { 0.0f, 0.0f, TILE_SIZE, TILE_SIZE };

                int tileID = -1; // Default to empty
                if (r >= 0 && c >= 0 && r < gridRows && c < currentGrid[r].size()) {
                    tileID = currentGrid[r][c];
                } else if (layer == 1) {
                    tileID = 0; // Draw void for out of bounds only on base layer
                }

                if (tileID >= 0) {
                    int index = tileID; // CSV IDs are already 0-based
                    int tileX = index % tilesetCols;
                    int tileY = index / tilesetCols;
                    sourceRec.x = (float)tileX * TILE_SIZE + 0.05f;
                    sourceRec.y = (float)tileY * TILE_SIZE + 0.05f;
                    sourceRec.width = TILE_SIZE - 0.1f;
                    sourceRec.height = TILE_SIZE - 0.1f;
                    DrawTexturePro(tileset, sourceRec, destRec, {0,0}, 0.0f, WHITE);
                }
            }
        }
    }

    for (auto* entity : levelEntities) {
        entity->Draw();
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
}

void LevelManager::AddEntity(GameObject* entity) {
    if (entity) {
        levelEntities.push_back(entity);
    }
}

bool LevelManager::IsSolidCollision(Rectangle box) const {
    // Find min and max tile indices that overlap with the box
    // Box might have negative coords if outside, so use floor
    int minCol = (int)std::floor((box.x + COLLISION_EDGE_PADDING) / TILE_SIZE);
    int maxCol = (int)std::floor((box.x + box.width - COLLISION_EDGE_PADDING) / TILE_SIZE);
    int minRow = (int)std::floor((box.y + COLLISION_EDGE_PADDING) / TILE_SIZE);
    int maxRow = (int)std::floor((box.y + box.height - COLLISION_EDGE_PADDING) / TILE_SIZE);

    for (int r = minRow; r <= maxRow; ++r) {
        for (int c = minCol; c <= maxCol; ++c) {
            int tileID1 = -1;
            int tileID2 = -1;
            MapObjectId mapObjectId = MapObjectId::Empty;
            if (r >= 0 && c >= 0 && r < gridRows) {
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
                        (float)c * TILE_SIZE + RIGHT_PARTIAL_WALL_OFFSET,
                        (float)r * TILE_SIZE,
                        PARTIAL_WALL_WIDTH,
                        TILE_SIZE
                    };
                    if (CheckCollisionRecs(box, solidPart)) {
                        return true;
                    }
                } else if (tileID >= 12 && tileID <= 14) {
                    // Left 9px solid
                    Rectangle solidPart = {
                        (float)c * TILE_SIZE,
                        (float)r * TILE_SIZE,
                        PARTIAL_WALL_WIDTH,
                        TILE_SIZE
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
    if (distance <= COLLISION_EDGE_PADDING) {
        return true;
    }

    float radius = std::max(projectileRadius, COLLISION_EDGE_PADDING);
    int probeCount = std::max(1, (int)std::ceil(distance / MAX_PROBE_SPACING));

    // Skip both endpoints: the first can overlap the shooter and the last is
    // expected to overlap the target. Intermediate probes test the shot path.
    for (int probeIndex = 1; probeIndex < probeCount; ++probeIndex) {
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
    return !IsSolidCollision(entity->GetBoundingBox());
}

Vector2 LevelManager::WorldToTile(Vector2 worldPos) const {
    return {
        std::floor(worldPos.x / TILE_SIZE),
        std::floor(worldPos.y / TILE_SIZE)
    };
}

Vector2 LevelManager::TileToWorld(int tileX, int tileY) const {
    return {
        tileX * TILE_SIZE + TILE_SIZE / 2.0f,
        tileY * TILE_SIZE + TILE_SIZE / 2.0f
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

Vector2 LevelManager::GetNextMoveTarget(
    Enemy& enemy,
    Vector2 fallbackTarget
) {
    return enemyPathManager.GetNextMoveTarget(
        *this,
        enemy,
        fallbackTarget
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
