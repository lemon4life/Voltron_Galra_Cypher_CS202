#include "Core/Manager/LevelManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/EntityFactory.h"

#include "Entities/Enemy.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace {
    constexpr float TILE_SIZE = 32.0f;
    constexpr float COLLISION_EDGE_PADDING = 0.001f;

    bool IsBlockingTileID(int tileID) {
        if (tileID == 0) return true;
        if (tileID >= 1 && tileID <= 14) return true;
        return false;
    }

    void CenterEntityOnPoint(GameObject* entity, Vector2 center) {
        if (!entity) return;

        Rectangle box = entity->GetBoundingBox();
        Vector2 position = entity->GetPosition();
        Vector2 boxCenter = {
            box.x + box.width / 2.0f,
            box.y + box.height / 2.0f
        };

        entity->SetPosition({
            position.x + center.x - boxCenter.x,
            position.y + center.y - boxCenter.y
        });
    }
}

LevelManager::LevelManager() : gridRows(0), gridCols(0), levelWidth(0.0f), levelHeight(0.0f) {
    tileset = LoadTexture("assets/tileset/Galra_ship_Tileset.png");
}

LevelManager::~LevelManager() {
    ClearLevel();
    UnloadTexture(tileset);
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
    gridCols = 0;

    bool isCSV = (filepath.length() >= 4 && filepath.substr(filepath.length() - 4) == ".csv");

    while (std::getline(file, line)) {
        if (line.length() > 0 && line.back() == '\r') {
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
                else if (type == 'N' || type == 'E') {
                    tileID = 20; // Map to Floor ID
                    Vector2 tileCenter = {
                        (float)c * TILE_SIZE + TILE_SIZE / 2.0f,
                        (float)mapGridLayer1.size() * TILE_SIZE + TILE_SIZE / 2.0f
                    };

                    GameObject* entity = EntityFactory::CreateEntity(type, tileCenter, teamManager);
                    if (entity != nullptr) {
                        levelEntities.push_back(entity);
                    }

                    // GameObject* entity = EntityFactory::CreateEntity(type, tileCenter, teamManager);
                    // if (entity != nullptr) {
                    //     CenterEntityOnPoint(entity, tileCenter);
                    //     if (IsValidSpawnLocation(entity)) {
                    //         AddEntity(entity);
                    //     } else {
                    //         delete entity;
                    //     }
                    // }
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
                    if (line.length() > 0 && line.back() == '\r') line.pop_back();
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
    levelWidth = gridCols * 32.0f;
    levelHeight = gridRows * 32.0f;

    // Entity spawning via LevelManager is temporarily disabled
    // because CSV map layer 1 only contains visual tile IDs.
    // Entities could be spawned via a separate Layer 2.

    // Store in GameManager for global access
    GameManager::GetInstance().SetLevelBounds(levelWidth, levelHeight);
}

void LevelManager::UpdateLevel(float deltaTime) {
    ProcessPendingRemovals();

    enemyPathManager.Update(this, deltaTime);

    for (auto it = levelEntities.begin(); it != levelEntities.end(); it++) {
        if (Enemy* e = dynamic_cast<Enemy*>(*it)) {
            if (e->IsDead()) {
                continue;
            }
        }

        (*it)->Update(deltaTime);
    }

    ProcessPendingRemovals();
}

void LevelManager::DrawLevel() {
    int padding = 20;

    // Calculate columns in the tileset (assuming each tile is 32x32)
    int tilesetCols = tileset.width / 32;
    if (tilesetCols <= 0) tilesetCols = 1; // Fallback to avoid division by zero

    for (int layer = 1; layer <= 2; ++layer) {
        const auto& currentGrid = (layer == 1) ? mapGridLayer1 : mapGridLayer2;
        if (currentGrid.empty()) continue;

        for (int r = -padding; r < gridRows + padding; ++r) {
            for (int c = -padding; c < gridCols + padding; ++c) {
                Rectangle destRec = { (float)c * 32.0f, (float)r * 32.0f, 32.0f, 32.0f };
                Rectangle sourceRec = { 0.0f, 0.0f, 32.0f, 32.0f };

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
                    sourceRec.x = (float)tileX * 32.0f;
                    sourceRec.y = (float)tileY * 32.0f;
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
    for (auto* entity : levelEntities) {
        if (Enemy* enemy = dynamic_cast<Enemy*>(entity)) {
            enemyPathManager.RemoveEnemy(enemy);
            enemy->RemoveObserver(this);
        }
        delete entity;
    }
    levelEntities.clear();
    pendingRemoval.clear();
    mapGridLayer1.clear();
    mapGridLayer2.clear();
}

void LevelManager::AddEntity(GameObject* entity) {
    if (entity) {
        if (Enemy* enemy = dynamic_cast<Enemy*>(entity)) {
            enemy->AddObserver(this);
        }
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
            if (r >= 0 && c >= 0 && r < gridRows) {
                if (r < mapGridLayer1.size() && c < mapGridLayer1[r].size()) tileID1 = mapGridLayer1[r][c];
                if (r < mapGridLayer2.size() && c < mapGridLayer2[r].size()) tileID2 = mapGridLayer2[r][c];
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
                    Rectangle solidPart = { (float)c * TILE_SIZE + 23.0f, (float)r * TILE_SIZE, 9.0f, TILE_SIZE };
                    if (CheckCollisionRecs(box, solidPart)) {
                        return true;
                    }
                } else if (tileID >= 12 && tileID <= 14) {
                    // Left 9px solid
                    Rectangle solidPart = { (float)c * TILE_SIZE, (float)r * TILE_SIZE, 9.0f, TILE_SIZE };
                    if (CheckCollisionRecs(box, solidPart)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool LevelManager::IsValidSpawnLocation(Vector2 position) const {
    // An enemy's bounding box is roughly 24x36, centered.
    Rectangle spawnBox = { position.x - 12.0f, position.y - 18.0f, 24.0f, 36.0f };
    return !IsSolidCollision(spawnBox);
}

bool LevelManager::IsValidSpawnLocation(const GameObject* entity) const {
    if (!entity) return false;
    return !IsSolidCollision(entity->GetBoundingBox());
}

bool LevelManager::IsWalkableTile(int x, int y) const {
    if (x < 0 || y < 0 || y >= gridRows || x >= gridCols) {
        return false;
    }

    int tileID1 = -1;
    int tileID2 = -1;

    if (y < (int)mapGridLayer1.size() && x < (int)mapGridLayer1[y].size()) {
        tileID1 = mapGridLayer1[y][x];
    }
    if (y < (int)mapGridLayer2.size() && x < (int)mapGridLayer2[y].size()) {
        tileID2 = mapGridLayer2[y][x];
    }

    return !IsBlockingTileID(tileID1) && !IsBlockingTileID(tileID2);
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
// Level Manager funcion for Enemy interaction
//////////////////////////////////////////////

void LevelManager::ProcessPendingRemovals() {
    for (Enemy* enemy : pendingRemoval) {
        enemyPathManager.RemoveEnemy(enemy);

        auto it = std::find(levelEntities.begin(), levelEntities.end(), enemy);
        if (it != levelEntities.end()) {
            enemy->RemoveObserver(this);
            delete *it;
            levelEntities.erase(it);
        }
    }
    pendingRemoval.clear();
}

void LevelManager::OnEnemyPathFind(Enemy* enemy) {
    enemyPathManager.AddEnemy(enemy);
}

void LevelManager::OnEnemyPathFindEnded(Enemy* enemy) {
    enemyPathManager.RemoveEnemy(enemy);
}

void LevelManager::OnEnemyDied(Enemy* enemy) {
    if (!enemy) return;

    if (std::find(pendingRemoval.begin(), pendingRemoval.end(), enemy) == pendingRemoval.end()) {
        pendingRemoval.push_back(enemy);
    }
}
