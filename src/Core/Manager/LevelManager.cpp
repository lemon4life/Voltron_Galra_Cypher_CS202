#include "Core/Manager/LevelManager.h"
#include "Core/Constants.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/DecalManager.h"
#include "Entities/Player/Paladin.h"
#include "Core/EntityFactory.h"
#include "Entities/EnemyEntities/EnemyDiver.h"
#include "Entities/EnemyEntities/Drone.h"
#include "Core/LevelAccess.h"
#include "Entities/Enemy.h"
#include "Entities/Props/Prop.h"
#include "Entities/Props/DoorGate.h"

#include "Core/Level/ILevelProvider.h"
#include "Entities/Props/Pot.h"
#include "Core/Level/StaticLevelProvider.h"
#include "Core/Utils/MapLoader.h"
#include "Core/Level/ProceduralLevelProvider.h"
#include "Core/Level/StaticLevelProvider.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <utility>
#include <unordered_set>

namespace {
    auto pos_hash = [](int x, int y) -> int {
        unsigned int h = (unsigned int)(x * 374761393 ^ y * 668265263);
        h = (h ^ (h >> 13)) * 1274126177;
        return h ^ (h >> 16);
    };
}

    constexpr float COLLISION_EDGE_PADDING = 0.001f;
    constexpr float PARTIAL_WALL_WIDTH = 9.0f;
    constexpr float RIGHT_PARTIAL_WALL_OFFSET = Constants::RENDER_TILE_SIZE - PARTIAL_WALL_WIDTH;
    constexpr int DRAW_PADDING_TILES = 20;

LevelManager::LevelManager()
    : levelWidth(0.0f), levelHeight(0.0f), gridRows(0), gridCols(0) {
    // Textures must be loaded after InitWindow() — call InitializeAssets() explicitly.
}

void LevelManager::InitializeAssets() {
    floorTileset = LoadTexture("assets/tileset/Galra_Floors.png");
    wallTileset = LoadTexture("assets/tileset/Galra_Walls.png");
    prop1Texture = LoadTexture("assets/Objects/tall_object_1_8.png");
    prop2Texture = LoadTexture("assets/Objects/object_2.png");
    boxTexture = LoadTexture("assets/Objects/box.png");
    gateTexture = LoadTexture("assets/Objects/Transfer_gate.png");

    SetTextureFilter(tileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(floorTileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(wallTileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(prop1Texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(prop2Texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(boxTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(gateTexture, TEXTURE_FILTER_POINT);
}

LevelManager::~LevelManager() {
    ClearLevel();
    UnloadTexture(tileset);
    UnloadTexture(floorTileset);
    UnloadTexture(wallTileset);
    UnloadTexture(prop1Texture);
    UnloadTexture(prop2Texture);
    UnloadTexture(boxTexture);
    UnloadTexture(gateTexture);
}

bool LevelManager::LoadObjectGrid(const std::string& filepath) {
    return MapLoader::ParseObjectGrid(filepath, mapObjectGrid, mapGridLayer1);
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

            Vector2 spawnPos = TileToWorld(column, row);
            
            AddEntity(EntityFactory::CreateEntity(
                objectId,
                spawnPos,
                { row, column },
                teamManager,
                GetLevelAccessBundle()
            ));

            if (objectId == MapObjectId::NPC) {
                Vector2 shiroPos = spawnPos;
                shiroPos.x -= Constants::RENDER_TILE_SIZE * 2; // Spawn 2 tiles to the left
                AddEntity(EntityFactory::CreateEntity(
                    MapObjectId::ShiroNPC,
                    shiroPos,
                    { row, column - 2 },
                    teamManager,
                    GetLevelAccessBundle()
                ));
            }
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

    if (!MapLoader::ParseCSV(filepath, mapGridLayer1)) {
        return;
    }

    std::string layer2Path = filepath;
    layer2Path.replace(layer2Path.find("Layer 1"), 7, "Layer 2");
    if (!MapLoader::ParseCSV(layer2Path, mapGridLayer2)) {
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
    
    // Spawn objects
    SpawnGameObjects(teamManager);

    // Store in GameManager for global access
    GameManager::GetInstance().SetLevelBounds(levelWidth, levelHeight);

    currentLevelProvider = std::make_unique<StaticLevelProvider>(
        mapGridLayer1, mapGridLayer2, mapObjectGrid,
        floorTileset, wallTileset, gridRows, gridCols
    );

    staticSpawnNodes.clear();
    float ts = Constants::RENDER_TILE_SIZE;
    for (float y = ts; y < levelHeight - ts; y += ts/2.0f) {
        for (float x = ts; x < levelWidth - ts; x += ts/2.0f) {
            if (!IsSolidCollision({x - 16.0f, y - 16.0f, 32.0f, 32.0f})) {
                staticSpawnNodes.push_back({x, y});
            }
        }
    }
}

void LevelManager::UpdateLevel(float deltaTime, Vector2 playerPos) {
    if (IsProceduralDungeon()) {
        if (currentlyLockedRoom && currentlyLockedRoom->state == RoomState::CLEARED) {
            for (auto* door : currentlyLockedRoom->doors) {
                door->SetState(DoorGate::State::OPENING);
            }
            if (!currentlyLockedRoom->doors.empty()) {
                AudioManager::GetInstance().PlaySoundEffect("fx_doorgate");
            }
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

                        for (auto* door : node->doors) {
                            door->SetState(DoorGate::State::CLOSING);
                        }
                        if (!node->doors.empty()) {
                            AudioManager::GetInstance().PlaySoundEffect("fx_doorgate");
                        }
                        MarkNavigationChanged();

                        // Nudge player to room center so they don't get stuck inside a door collider
                        Rectangle bounds = node->GetWorldBounds();
                        float roomCenterX = bounds.x + bounds.width / 2.0f;
                        float roomCenterY = bounds.y + bounds.height / 2.0f;
                        
                        // Move toward center, but only if currently overlapping a door
                        bool overlappingDoor = false;
                        for (auto* door : node->doors) {
                            if (CheckCollisionRecs(playerBox, door->GetBoundingBox())) {
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
        if ((*it)->GetObjectType() == GameObjectType::Prop) {
            class Pot* pot = dynamic_cast<class Pot*>(*it);
            if (pot && pot->IsConsumed()) {
                QueueRemoval(pot);
                continue;
            }
        }

        if ((*it)->GetObjectType() == GameObjectType::Enemy) {
            Enemy* e = static_cast<Enemy*>(*it);
            if (e->IsDead()) {
                Texture2D downTex = AssetManager::GetInstance().GetTexture("Enemy_Down");
                if (dynamic_cast<Drone*>(e)) {
                    downTex = AssetManager::GetInstance().GetTexture("Drone_down");
                }
                DecalManager::GetInstance().AddCorpse(e->GetPosition(), downTex, e->IsFacingLeft(), e->GetKnockbackVelocity());

                GameManager::GetInstance().SpawnQuintessenceOrb(e->GetPosition());

                int randNum = GetRandomValue(0, 1);
                if (dynamic_cast<Drone*>(e)) {
                    AudioManager::GetInstance().PlaySoundEffectVolume("drone_dead_" + std::to_string(randNum), 0.25f);
                } else {
                    AudioManager::GetInstance().PlaySoundEffectVolume("knight_dead_" + std::to_string(randNum), 0.25f);
                }

                QueueRemoval(e);
                continue;
            }
        }

        DoorGate* updatingDoor =
            (*it)->GetObjectType() == GameObjectType::DoorGate
                ? static_cast<DoorGate*>(*it)
                : nullptr;
        bool doorWasSolid = updatingDoor && updatingDoor->IsSolid();
        (*it)->Update(deltaTime);
        if (updatingDoor && doorWasSolid != updatingDoor->IsSolid()) {
            MarkNavigationChanged();
        }
    }

    // DecalManager now handles corpse physics

    ProcessPendingAdditions();
    ProcessPendingMapObjectDestructions();
    ProcessPendingRemovals();
}

void LevelManager::DrawLevelBase() {
    if (currentLevelProvider) {
        currentLevelProvider->DrawBase();
    }

    
    // Draw base layer for dynamic props
    for (auto* entity : levelEntities) {
        if (entity->GetObjectType() == GameObjectType::Box) {
            static_cast<Prop*>(entity)->DrawBaseLayer();
        } else if (entity->GetObjectType() == GameObjectType::DoorGate) {
            static_cast<DoorGate*>(entity)->DrawBaseLayer();
        }
    }

    // DecalManager now handles corpse rendering
}

void LevelManager::GetDepthRenderItems(std::vector<DepthRenderItem>& items) {
    if (currentLevelProvider) {
        currentLevelProvider->GetDepthRenderItems(items);
    }

    for (auto* entity : levelEntities) {
        if (entity->GetObjectType() == GameObjectType::Box) {
            static_cast<Prop*>(entity)->AddDepthRenderItems(items);
        } else if (entity->GetObjectType() == GameObjectType::DoorGate) {
            static_cast<DoorGate*>(entity)->AddDepthRenderItems(items);
        } else {
            items.push_back({
                entity->GetBoundingBox().y + entity->GetBoundingBox().height,
                [entity]() { entity->Draw(); }
            });
        }
    }
}

void LevelManager::ClearLevel() {
    pendingMapObjectDestructions.clear();
    pendingRemoval.clear();
    enemyPathManager.Clear();

    for (GameObject* entity : pendingAddition) {
        delete entity;
    }
    pendingAddition.clear();

    for (auto* entity : levelEntities) {
        delete entity;
    }
    levelEntities.clear();
    DecalManager::GetInstance().Clear();
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
    MarkNavigationChanged();
}

void LevelManager::AddEntity(GameObject* entity) {
    if (entity) {
        levelEntities.push_back(entity);
        if (entity && entity->IsSolidNavigationObstacle()) {
            MarkNavigationChanged();
        }
    }
}

bool LevelManager::QueueEnemySpawn(
    MapObjectId enemyType,
    Vector2 position,
    TeamManager* teamManager
) {
    if (enemyType != MapObjectId::Chaser &&
        enemyType != MapObjectId::Range &&
        enemyType != MapObjectId::Diver) {
        return false;
    }

    GameObject* entity = EntityFactory::CreateEntity(
        enemyType,
        position,
        { -1, -1 },
        teamManager,
        GetLevelAccessBundle()
    );
    if (!entity) {
        return false;
    }

    pendingAddition.push_back(entity);
    return true;
}

void LevelManager::ProcessPendingAdditions() {
    for (GameObject* entity : pendingAddition) {
        AddEntity(entity);
    }
    pendingAddition.clear();
}

bool LevelManager::IsSolidCollision(Rectangle box, bool ignoreProps) const {
    if (currentLevelProvider) {
        return currentLevelProvider->IsSolidCollision(box, ignoreProps);
    }
    return false;
}

bool LevelManager::IsSolidMapObject(MapObjectId objectId) const {
    if (objectId == MapObjectId::DestructibleBox || objectId == MapObjectId::Prop1 || objectId == MapObjectId::Prop2 || objectId == MapObjectId::MockWall) {
        return true; // DestructibleBox occupies one solid cell.
    }

    return false;
}

bool LevelManager::HasClearLineOfSight(
    Vector2 start,
    Vector2 end,
    float projectileRadius
) const {
    float tileW = Constants::RENDER_TILE_SIZE;
    float radius = std::max(projectileRadius, COLLISION_EDGE_PADDING);

    if (IsProceduralDungeon() && activeRoom) {
        int x0 = (int)std::floor((start.x - roomOffset.x) / tileW);
        int y0 = (int)std::floor((start.y - roomOffset.y) / tileW);
        int x1 = (int)std::floor((end.x - roomOffset.x) / tileW);
        int y1 = (int)std::floor((end.y - roomOffset.y) / tileW);

        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;

        while (true) {
            if (y0 >= 0 && y0 < activeRoom->height && x0 >= 0 && x0 < activeRoom->width) {
                int tile = activeRoom->layer0_tiles[y0][x0];
                if (tile == 1 || tile == 2) return false;
            } else {
                return false;
            }
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    } else {
        int x0 = (int)std::floor(start.x / tileW);
        int y0 = (int)std::floor(start.y / tileW);
        int x1 = (int)std::floor(end.x / tileW);
        int y1 = (int)std::floor(end.y / tileW);

        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;

        while (true) {
            if (y0 >= 0 && y0 < gridRows && x0 >= 0 && x0 < gridCols) {
                int t1 = 0, t2 = 0;
                if (y0 < (int)mapGridLayer1.size() && x0 < (int)mapGridLayer1[y0].size()) t1 = mapGridLayer1[y0][x0];
                if (y0 < (int)mapGridLayer2.size() && x0 < (int)mapGridLayer2[y0].size()) t2 = mapGridLayer2[y0][x0];
                if (t1 == 0 || t2 == 0) return false;
                if (t1 >= 4 && t1 <= 11) return false;
                if (t2 >= 4 && t2 <= 11) return false;
                if ((t1 >= 1 && t1 <= 3) || (t2 >= 1 && t2 <= 3) || (t1 >= 12 && t1 <= 14) || (t2 >= 12 && t2 <= 14)) return false;
            } else {
                return false;
            }
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    constexpr float MAX_PROBE_SPACING = 8.0f;
    Vector2 segment = { end.x - start.x, end.y - start.y };
    float distance = std::sqrt(segment.x * segment.x + segment.y * segment.y);
    int probeCount = std::max(1, (int)std::ceil(distance / MAX_PROBE_SPACING));

    for (int probeIndex = 0; probeIndex <= probeCount; ++probeIndex) {
        float amount = (float)probeIndex / (float)probeCount;
        Vector2 point = { start.x + segment.x * amount, start.y + segment.y * amount };
        Rectangle probe = { point.x - radius, point.y - radius, radius * 2.0f, radius * 2.0f };

        for (const auto& entity : levelEntities) {
            if (entity && entity->IsSolidNavigationObstacle()) {
                if (CheckCollisionRecs(probe, entity->GetCollisionBox())) {
                    return false;
                }
            }
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

        if (IsProceduralDungeon()) {
            // In procedural dungeons, we just remove the entity. No mapObjectGrid exists to update.
            QueueRemoval(object);
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
    if (pendingRemoval.empty()) return;

    bool navChanged = false;

    levelEntities.erase(
        std::remove_if(levelEntities.begin(), levelEntities.end(),
            [this, &navChanged](GameObject* entity) {
                if (pendingRemoval.count(entity)) {
                    if (entity->IsSolidNavigationObstacle()) {
                        navChanged = true;
                    }
                    delete entity;
                    return true;
                }
                return false;
            }),
        levelEntities.end()
    );

    if (navChanged) {
        MarkNavigationChanged();
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
        return currentlyLockedRoom->GetWorldBounds();
    }
    return { 0.0f, 0.0f, levelWidth, levelHeight };
}

bool LevelManager::IsPlayerInExitRoom(Vector2 playerPos) const {
    if (!IsProceduralDungeon()) return false;
    
    Rectangle playerBox = { playerPos.x - 8, playerPos.y - 8, 16, 16 };
    for (const auto& node : levelMap.generatedNodes) {
        if (node->type == RoomType::EXIT) {
            Rectangle bounds = node->GetWorldBounds();
            float roomCenterX = bounds.x + bounds.width / 2.0f;
            float roomCenterY = bounds.y + bounds.height / 2.0f;
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
    if (entity) {
        pendingRemoval.insert(entity);
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

    if (!IsProceduralDungeon()) {
        if (cell.row < 0 || cell.column < 0 ||
            cell.row >= (int)mapObjectGrid.size() ||
            cell.column >= (int)mapObjectGrid[cell.row].size()) {
            return;
        }

        if (mapObjectGrid[cell.row][cell.column]
            != MapObjectId::DestructibleBox) {
            return;
        }
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

Rectangle RoomNode::GetWorldBounds() const {
    float tileW = Constants::RENDER_TILE_SIZE;
    int roomOuterSize = Constants::MAX_ROOM_TILE_SIZE + Constants::CORRIDOR_LENGTH;
    
    int currentRoomSize = this->roomSize;
    
    int offset = (Constants::MAX_ROOM_TILE_SIZE - currentRoomSize) / 2;
                          
    float startX = (gridX * roomOuterSize + offset) * tileW;
    float startY = (gridY * roomOuterSize + offset) * tileW;
    float roomW = currentRoomSize * tileW;
    float roomH = currentRoomSize * tileW;
    
    return { startX + tileW, startY + tileW, roomW - 2 * tileW, roomH - 2 * tileW };
}

void RoomNode::CalculateWalkableGrid(LevelManager* lm) {
    availableSpawnNodes.clear();
    
    Rectangle bounds = GetWorldBounds();
    
    // We only care about the physical room interior (inset by 1 tile from the interior bounds to avoid spawning on the edges)
    float tileW = Constants::RENDER_TILE_SIZE;
    float minX = bounds.x + tileW;
    float maxX = bounds.x + bounds.width - 2.0f * tileW;
    float minY = bounds.y + tileW;
    float maxY = bounds.y + bounds.height - 2.0f * tileW;

    // Use a grid size of half a tile for high resolution safe-spot finding
    float gridSize = tileW / 2.0f;
    for (float y = minY; y <= maxY; y += gridSize) {
        for (float x = minX; x <= maxX; x += gridSize) {
            // Hitbox for enemy spawning (typical 32x32 size for standard enemies)
            Rectangle hitbox = { x - 16.0f, y - 16.0f, 32.0f, 32.0f };
            if (!lm->IsSolidCollision(hitbox)) {
                availableSpawnNodes.push_back({ x, y });
            }
        }
    }
}

void LevelManager::GenerateDungeon(TeamManager* teamManager) {
    printf("GenerateDungeon: Start\n");
    ClearLevel();
    printf("GenerateDungeon: Cleared level\n");
    levelMode = LevelMode::Procedural;
    currentlyLockedRoom = nullptr;

    printf("GenerateDungeon: Generating map\n");
    levelMap.Generate(7, 7);
    printf("GenerateDungeon: Baking level\n");
    activeRoom = levelMap.BakeLevel();
    printf("GenerateDungeon: Map baked\n");
    roomOffset = {0.0f, 0.0f};
    
    printf("GenerateDungeon: Generating walls\n");
    currentRoomWalls = activeRoom->GenerateWallColliders(roomOffset, Constants::RENDER_TILE_SIZE, 1.0f);
    printf("GenerateDungeon: Walls generated\n");
    
    levelWidth = activeRoom->width * Constants::RENDER_TILE_SIZE;
    levelHeight = activeRoom->height * Constants::RENDER_TILE_SIZE;
    GameManager::GetInstance().SetLevelBounds(levelWidth, levelHeight);

    // Teleport player to spawn room center
    if (levelMap.spawnRoom) {
        Rectangle bounds = levelMap.spawnRoom->GetWorldBounds();
        float spawnWorldX = bounds.x + bounds.width / 2.0f;
        float spawnWorldY = bounds.y + bounds.height / 2.0f;
        teamManager->GetActivePaladin()->SetPosition({spawnWorldX, spawnWorldY});
        teamManager->StartSpawnAnimation();

        // Add 3 testing pots
        AddEntity(new HpPot({spawnWorldX - 40.0f, spawnWorldY - 50.0f}));
        AddEntity(new ExPot({spawnWorldX, spawnWorldY - 50.0f}));
        AddEntity(new QuintPot({spawnWorldX + 40.0f, spawnWorldY - 50.0f}));

        // Auto-discover spawn room and mark it cleared (no combat in spawn)
        levelMap.spawnRoom->isDiscovered = true;
        levelMap.spawnRoom->state = RoomState::CLEARED;
    }

    printf("GenerateDungeon: Spawning props\n");
    // Instantiate procedural props as actual game entities
    if (activeRoom) {
        for (int y = 0; y < activeRoom->height; ++y) {
            for (int x = 0; x < activeRoom->width; ++x) {
                int propId = activeRoom->layer2_props[y][x];
                if (propId > 0) {
                    MapObjectId type = MapObjectId::Empty;
                    if (propId == 5) type = MapObjectId::DestructibleBox;
                    else if (propId == 6) type = MapObjectId::Prop2;
                    else if (propId == 7) type = MapObjectId::PotEX;
                    else if (propId == 8) type = MapObjectId::PotHP;
                    else if (propId == 9) type = MapObjectId::PotQuint;
                    else if (propId == 10) type = MapObjectId::Prop1;

                    if (type != MapObjectId::Empty) {
                        Vector2 worldPos = {
                            (float)x * Constants::RENDER_TILE_SIZE + Constants::RENDER_TILE_SIZE / 2.0f,
                            (float)y * Constants::RENDER_TILE_SIZE + Constants::RENDER_TILE_SIZE / 2.0f
                        };
                        AddEntity(EntityFactory::CreateEntity(
                            type,
                            worldPos,
                            {y, x}, // cell
                            teamManager,
                            GetLevelAccessBundle()
                        ));
                    }
                }
            }
        }
        
        printf("GenerateDungeon: Spawning doors\n");
        // Instantiate DoorGates for each RoomNode
        for (auto& node : levelMap.generatedNodes) {
            Rectangle bounds = node->GetWorldBounds();
            float tileW = Constants::RENDER_TILE_SIZE;
            
            int gridStartX = (int)((bounds.x - tileW) / tileW);
            int gridStartY = (int)((bounds.y - tileW) / tileW);
            int currentRoomSize = (int)(bounds.width / tileW) + 2;
            
            for (int y = 0; y < currentRoomSize; ++y) {
                for (int x = 0; x < currentRoomSize; ++x) {
                    if (activeRoom->layer1_objects[gridStartY + y][gridStartX + x] == 20) {
                        Vector2 worldPos = {
                            (gridStartX + x) * tileW,
                            (gridStartY + y) * tileW
                        };
                        DoorGate* door = new DoorGate(worldPos);
                        AddEntity(door);
                        node->doors.push_back(door);
                    }
                }
            }
            
            // Calculate walkable grid for this room now that all entities (props, doors) are spawned
            node->CalculateWalkableGrid(this);
        }
    }

    currentLevelProvider = std::make_unique<ProceduralLevelProvider>(
        activeRoom, roomOffset, levelEntities,
        floorTileset, wallTileset, prop1Texture, prop2Texture,
        boxTexture, gateTexture, levelMap
    );

    printf("GenerateDungeon: Done\n");
}

bool LevelManager::GetSafeSpawnPosition(std::shared_ptr<RoomNode> room, Vector2& outPos) {
    if (!room) return false;

    float ts = Constants::RENDER_TILE_SIZE;
    Rectangle bounds = room->GetWorldBounds();
    
    float startX = bounds.x + ts;
    float endX = bounds.x + bounds.width - ts;
    float startY = bounds.y + ts;
    float endY = bounds.y + bounds.height - ts;

    std::vector<Vector2> validSpots;
    for (float y = startY; y < endY; y += ts/2.0f) {
        for (float x = startX; x < endX; x += ts/2.0f) {
            if (!IsSolidCollision({x - 16.0f, y - 16.0f, 32.0f, 32.0f})) {
                validSpots.push_back({x, y});
            }
        }
    }
    
    if (validSpots.empty()) return false;
    
    outPos = validSpots[GetRandomValue(0, validSpots.size() - 1)];
    return true;
}

bool LevelManager::GetGuaranteedSpawnPoint(Vector2& outPos) {
    if (IsProceduralDungeon()) {
        return GetSafeSpawnPosition(currentlyLockedRoom, outPos);
    } else {
        if (staticSpawnNodes.empty()) return false;
        int index = GetRandomValue(0, staticSpawnNodes.size() - 1);
        outPos = staticSpawnNodes[index];
        // Pop to prevent overlapping spawns
        std::swap(staticSpawnNodes[index], staticSpawnNodes.back());
        staticSpawnNodes.pop_back();
        return true;
    }
}
