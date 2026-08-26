#include "Core/Manager/LevelManager.h"
#include "Core/Constants.h"
#include "Core/MapObjectFactory.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/LevelAccess.h"
#include "Entities/Props/DoorGate.h"

#include "Core/Level/ILevelProvider.h"
#include "Core/Level/StaticLevelProvider.h"
#include "Core/Utils/MapLoader.h"
#include "Core/Utils/LineOfSightGeometry.h"
#include "Core/Level/ProceduralLevelProvider.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <utility>
#include <unordered_set>
#include <limits>

namespace {
    constexpr float COLLISION_EDGE_PADDING = 0.001f;
    constexpr float GATE_ESCAPE_STEP = 1.0f;
    constexpr float GATE_ESCAPE_PADDING = 1.0f;
}

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

    SetTextureFilter(floorTileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(wallTileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(prop1Texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(prop2Texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(boxTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(gateTexture, TEXTURE_FILTER_POINT);
}

LevelManager::~LevelManager() {
    ClearLevel();
    ShutdownAssets();
}

void LevelManager::ShutdownAssets() {
    if (floorTileset.id != 0) UnloadTexture(floorTileset);
    if (wallTileset.id != 0) UnloadTexture(wallTileset);
    if (prop1Texture.id != 0) UnloadTexture(prop1Texture);
    if (prop2Texture.id != 0) UnloadTexture(prop2Texture);
    if (boxTexture.id != 0) UnloadTexture(boxTexture);
    if (gateTexture.id != 0) UnloadTexture(gateTexture);
    floorTileset = {};
    wallTileset = {};
    prop1Texture = {};
    prop2Texture = {};
    boxTexture = {};
    gateTexture = {};
}

bool LevelManager::LoadObjectGrid(const std::string& filepath) {
    return MapLoader::ParseObjectGrid(filepath, mapObjectGrid, mapGridLayer1);
}

DynamicSpawnList LevelManager::SpawnMapContent() {
    DynamicSpawnList dynamicSpawns;
    for (int row = 0; row < (int)mapObjectGrid.size(); ++row) {
        for (int column = 0;
             column < (int)mapObjectGrid[row].size();
             ++column) {
            MapObjectId objectId = mapObjectGrid[row][column];
            if (objectId == MapObjectId::Empty) {
                continue;
            }

            Vector2 position = TileToWorld(column, row);
            if (MapObjectFactory::IsMapObjectType(objectId)) {
                AddMapObject(MapObjectFactory::Create(
                    objectId,
                    position,
                    { row, column }
                ));
            } else {
                dynamicSpawns.push_back({
                    objectId,
                    position,
                    { row, column }
                });
                if (objectId == MapObjectId::NPC) {
                    Vector2 shiroPos = position;
                    shiroPos.x -= Constants::RENDER_TILE_SIZE * 2;
                    dynamicSpawns.push_back({
                        MapObjectId::ShiroNPC,
                        shiroPos,
                        { row, column - 2 }
                    });
                }
            }
        }
    }
    return dynamicSpawns;
}

DynamicSpawnList LevelManager::LoadLevel(const std::string& filepath) {
    DynamicSpawnList dynamicSpawns;
    ClearLevel();
    levelMode = LevelMode::Layered;

    if (filepath.find("Layer 1") == std::string::npos ||
        filepath.size() < 4 ||
        filepath.substr(filepath.size() - 4) != ".csv") {
        std::cerr << "Level path must reference a Layer 1 CSV: " << filepath << std::endl;
        return dynamicSpawns;
    }

    if (!MapLoader::ParseCSV(filepath, mapGridLayer1)) {
        return dynamicSpawns;
    }

    std::string layer2Path = filepath;
    layer2Path.replace(layer2Path.find("Layer 1"), 7, "Layer 2");
    if (!MapLoader::ParseCSV(layer2Path, mapGridLayer2)) {
        ClearLevel();
        return dynamicSpawns;
    }

    if (mapGridLayer2.size() != mapGridLayer1.size() ||
        mapGridLayer2.front().size() != mapGridLayer1.front().size()) {
        std::cerr << "Layer 2 dimensions do not match Layer 1: " << layer2Path << std::endl;
        ClearLevel();
        return dynamicSpawns;
    }

    if (!LoadObjectGrid(filepath)) {
        ClearLevel();
        return dynamicSpawns;
    }

    gridRows = static_cast<int>(mapGridLayer1.size());
    gridCols = static_cast<int>(mapGridLayer1.front().size());
    levelWidth = gridCols * Constants::RENDER_TILE_SIZE;
    levelHeight = gridRows * Constants::RENDER_TILE_SIZE;

    // Spawn objects
    DynamicSpawnList gridSpawns = SpawnMapContent();
    dynamicSpawns.insert(
        dynamicSpawns.end(),
        gridSpawns.begin(),
        gridSpawns.end()
    );

    currentLevelProvider = std::make_unique<StaticLevelProvider>(
        mapGridLayer1, mapGridLayer2,
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
    return dynamicSpawns;
}

void LevelManager::UpdateLevel(
    float deltaTime,
    Vector2 playerPos,
    Rectangle playerCollisionBox
) {
    lineOfSightDebugTraces.clear();

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

                        Vector2 escapePosition;
                        if (FindGateEscapePosition(
                                playerCollisionBox,
                                playerPos,
                                escapePosition)) {
                            nudgePosition = escapePosition;
                            needsNudge = true;
                        }

                        break;
                    }
                }
            }
        }
    }

    for (const std::unique_ptr<MapObject>& object : mapObjects) {
        if (!object) continue;
        bool wasSolid = object->IsSolid();
        object->Update(deltaTime);
        if (wasSolid != object->IsSolid()) {
            MarkNavigationChanged();
        }
    }
    ProcessDestroyedMapObjects();
}

bool LevelManager::FindGateEscapePosition(
    Rectangle playerCollisionBox,
    Vector2 playerPosition,
    Vector2& escapePosition
) const {
    if (!currentlyLockedRoom ||
        playerCollisionBox.width <= 0.0f ||
        playerCollisionBox.height <= 0.0f) {
        return false;
    }

    auto overlapsLockedGate = [&](Rectangle bounds) {
        for (DoorGate* door : currentlyLockedRoom->doors) {
            if (door && door->IsSolid() &&
                CheckCollisionRecs(bounds, door->GetCollisionBox())) {
                return true;
            }
        }
        return false;
    };

    if (!overlapsLockedGate(playerCollisionBox)) {
        return false;
    }

    Rectangle roomBounds = currentlyLockedRoom->GetWorldBounds();
    Vector2 roomCenter = {
        roomBounds.x + roomBounds.width * 0.5f,
        roomBounds.y + roomBounds.height * 0.5f
    };
    Vector2 towardCenter = {
        roomCenter.x - playerPosition.x,
        roomCenter.y - playerPosition.y
    };
    float distanceToCenter = std::sqrt(
        towardCenter.x * towardCenter.x + towardCenter.y * towardCenter.y
    );
    if (distanceToCenter <= COLLISION_EDGE_PADDING) {
        return false;
    }
    towardCenter.x /= distanceToCenter;
    towardCenter.y /= distanceToCenter;

    Vector2 collisionOffset = {
        playerCollisionBox.x - playerPosition.x,
        playerCollisionBox.y - playerPosition.y
    };
    auto collisionBoxAt = [&](Vector2 position) {
        return Rectangle{
            position.x + collisionOffset.x,
            position.y + collisionOffset.y,
            playerCollisionBox.width,
            playerCollisionBox.height
        };
    };

    float maximumPushDistance = distanceToCenter;
    for (float pushDistance = GATE_ESCAPE_STEP;
         pushDistance <= maximumPushDistance;
         pushDistance += GATE_ESCAPE_STEP) {
        Vector2 candidate = {
            playerPosition.x + towardCenter.x * pushDistance,
            playerPosition.y + towardCenter.y * pushDistance
        };
        Rectangle candidateBox = collisionBoxAt(candidate);
        if (overlapsLockedGate(candidateBox) ||
            IsSolidCollision(candidateBox)) {
            continue;
        }

        Vector2 paddedCandidate = {
            candidate.x + towardCenter.x * GATE_ESCAPE_PADDING,
            candidate.y + towardCenter.y * GATE_ESCAPE_PADDING
        };
        Rectangle paddedBox = collisionBoxAt(paddedCandidate);
        escapePosition = !overlapsLockedGate(paddedBox) &&
                !IsSolidCollision(paddedBox)
            ? paddedCandidate
            : candidate;
        return true;
    }

    Rectangle centerBox = collisionBoxAt(roomCenter);
    if (!overlapsLockedGate(centerBox) && !IsSolidCollision(centerBox)) {
        escapePosition = roomCenter;
        return true;
    }
    return false;
}

void LevelManager::DrawLevelBase() {
    if (currentLevelProvider) {
        currentLevelProvider->DrawBase();
    }

    
    for (const std::unique_ptr<MapObject>& object : mapObjects) {
        if (object) object->DrawBaseLayer();
    }
}

void LevelManager::GetDepthRenderItems(std::vector<DepthRenderItem>& items) {
    if (currentLevelProvider) {
        currentLevelProvider->GetDepthRenderItems(items);
    }

    for (const std::unique_ptr<MapObject>& object : mapObjects) {
        if (object) object->AddDepthRenderItems(items);
    }
}

void LevelManager::ClearLevel() {
    for (const std::shared_ptr<RoomNode>& node : levelMap.generatedNodes) {
        if (node) node->doors.clear();
    }
    mapObjects.clear();
    levelMap = LevelMap{};
    mapGridLayer1.clear();
    mapGridLayer2.clear();
    mapObjectGrid.clear();
    activeRoom = nullptr;
    currentlyLockedRoom = nullptr;
    roomOffset = {0.0f, 0.0f};
    nudgePosition = {0.0f, 0.0f};
    needsNudge = false;
    levelMode = LevelMode::Layered;
    currentLevelProvider.reset();
    MarkNavigationChanged();
}

MapObject* LevelManager::AddMapObject(
    std::unique_ptr<MapObject> object
) {
    if (!object) return nullptr;
    MapObject* pointer = object.get();
    bool isSolid = object->IsSolid();
    mapObjects.push_back(std::move(object));
    if (isSolid) MarkNavigationChanged();
    return pointer;
}

bool LevelManager::IsSolidCollision(Rectangle box) const {
    if (currentLevelProvider &&
        currentLevelProvider->IsSolidCollision(box)) {
        return true;
    }
    return FindSolidMapObjectCollision(box) != nullptr;
}

CollisionMovementResult LevelManager::ResolveSolidMovement(
    Rectangle collisionBox,
    Vector2 desiredDisplacement
) const {
    return CollisionMovement::ResolveSlide(
        collisionBox,
        desiredDisplacement,
        [this](Rectangle candidate) {
            return IsSolidCollision(candidate);
        }
    );
}

MapObject* LevelManager::FindSolidMapObjectCollision(
    Rectangle box
) const {
    EnsureLineOfSightBlockerIndex();
    Vector2 origin = GetLineOfSightGridOrigin();
    float tileSize = Constants::RENDER_TILE_SIZE;
    int minimumTileX = (int)std::floor(
        (box.x - origin.x) / tileSize
    );
    int maximumTileX = (int)std::floor(
        (box.x + box.width - COLLISION_EDGE_PADDING - origin.x) /
            tileSize
    );
    int minimumTileY = (int)std::floor(
        (box.y - origin.y) / tileSize
    );
    int maximumTileY = (int)std::floor(
        (box.y + box.height - COLLISION_EDGE_PADDING - origin.y) /
            tileSize
    );
    std::unordered_set<MapObject*> tested;
    for (int tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
        for (int tileX = minimumTileX; tileX <= maximumTileX; ++tileX) {
            auto entry = lineOfSightDynamicBlockers.find({ tileX, tileY });
            if (entry == lineOfSightDynamicBlockers.end()) continue;
            for (MapObject* object : entry->second) {
                if (!object || !tested.insert(object).second ||
                    !object->IsSolid()) {
                    continue;
                }
                if (CheckCollisionRecs(box, object->GetCollisionBox())) {
                    return object;
                }
            }
        }
    }
    return nullptr;
}

Vector2 LevelManager::GetLineOfSightGridOrigin() const {
    return IsProceduralDungeon() && activeRoom
        ? roomOffset
        : Vector2{ 0.0f, 0.0f };
}

LevelManager::LineOfSightTile LevelManager::WorldToLineOfSightTile(
    Vector2 position
) const {
    Vector2 origin = GetLineOfSightGridOrigin();
    float tileSize = Constants::RENDER_TILE_SIZE;
    return {
        (int)std::floor((position.x - origin.x) / tileSize),
        (int)std::floor((position.y - origin.y) / tileSize)
    };
}

Rectangle LevelManager::GetLineOfSightTileBounds(
    LineOfSightTile tile
) const {
    Vector2 origin = GetLineOfSightGridOrigin();
    float tileSize = Constants::RENDER_TILE_SIZE;
    return {
        origin.x + tile.x * tileSize,
        origin.y + tile.y * tileSize,
        tileSize,
        tileSize
    };
}

void LevelManager::EnsureLineOfSightBlockerIndex() const {
    Vector2 origin = GetLineOfSightGridOrigin();
    const RoomTemplate* room = IsProceduralDungeon()
        ? activeRoom.get()
        : nullptr;
    if (lineOfSightBlockerIndexValid &&
        lineOfSightBlockerIndexRevision == navigationRevision &&
        lineOfSightBlockerIndexOrigin.x == origin.x &&
        lineOfSightBlockerIndexOrigin.y == origin.y &&
        lineOfSightBlockerIndexRoom == room) {
        return;
    }

    lineOfSightDynamicBlockers.clear();
    float tileSize = Constants::RENDER_TILE_SIZE;
    for (const std::unique_ptr<MapObject>& object : mapObjects) {
        if (!object || !object->IsSolid()) continue;

        Rectangle bounds = object->GetCollisionBox();
        int minimumTileX = (int)std::floor(
            (bounds.x - origin.x) / tileSize
        );
        int maximumTileX = (int)std::floor(
            (bounds.x + bounds.width - COLLISION_EDGE_PADDING - origin.x) /
                tileSize
        );
        int minimumTileY = (int)std::floor(
            (bounds.y - origin.y) / tileSize
        );
        int maximumTileY = (int)std::floor(
            (bounds.y + bounds.height - COLLISION_EDGE_PADDING - origin.y) /
                tileSize
        );

        for (int tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
            for (int tileX = minimumTileX;
                 tileX <= maximumTileX;
                 ++tileX) {
                lineOfSightDynamicBlockers[{ tileX, tileY }].push_back(
                    object.get()
                );
            }
        }
    }

    lineOfSightBlockerIndexValid = true;
    lineOfSightBlockerIndexRevision = navigationRevision;
    lineOfSightBlockerIndexOrigin = origin;
    lineOfSightBlockerIndexRoom = room;
}

bool LevelManager::HasClearLineOfSight(
    Vector2 start,
    Vector2 end,
    float projectileRadius
) const {
    bool recordDebug = Constants::DEBUG_DRAW_LINE_OF_SIGHT;
    LineOfSightDebugTrace debugTrace;
    debugTrace.start = start;
    debugTrace.end = end;
    debugTrace.radius = std::max(0.0f, projectileRadius);

    auto finishQuery = [&](bool clear) {
        if (recordDebug) {
            debugTrace.clear = clear;
            lineOfSightDebugTraces.push_back(std::move(debugTrace));
        }
        return clear;
    };

    if (!currentLevelProvider ||
        !std::isfinite(start.x) || !std::isfinite(start.y) ||
        !std::isfinite(end.x) || !std::isfinite(end.y) ||
        !std::isfinite(projectileRadius)) {
        return finishQuery(false);
    }

    float radius = std::max(0.0f, projectileRadius);
    float tileSize = Constants::RENDER_TILE_SIZE;
    float spanAsFloat = std::ceil(radius / tileSize);
    if (spanAsFloat > 4096.0f) return finishQuery(false);
    int neighborSpan = (int)spanAsFloat;
    EnsureLineOfSightBlockerIndex();

    std::unordered_set<LineOfSightTile, LineOfSightTileHash>
        visitedCenterTiles;
    std::unordered_set<LineOfSightTile, LineOfSightTileHash>
        visitedCandidateTiles;
    std::unordered_set<const MapObject*> testedMapBlockers;
    std::vector<Rectangle> staticColliders;
    staticColliders.reserve(2);

    auto testCollider = [&](Rectangle collider) {
        int colliderIndex = -1;
        if (recordDebug) {
            colliderIndex = (int)debugTrace.testedColliders.size();
            debugTrace.testedColliders.push_back(collider);
        }
        bool intersects =
            LineOfSightGeometry::CapsuleIntersectsRectangle(
                start,
                end,
                radius,
                collider
            );
        if (recordDebug && intersects) {
            debugTrace.blockingColliderIndex = colliderIndex;
        }
        return intersects;
    };

    auto visitCenterTile = [&](LineOfSightTile centerTile) {
        if (!visitedCenterTiles.insert(centerTile).second) return false;
        if (recordDebug) {
            debugTrace.ddaTiles.push_back(
                GetLineOfSightTileBounds(centerTile)
            );
        }

        for (int offsetY = -neighborSpan;
             offsetY <= neighborSpan;
             ++offsetY) {
            for (int offsetX = -neighborSpan;
                 offsetX <= neighborSpan;
                 ++offsetX) {
                LineOfSightTile candidate = {
                    centerTile.x + offsetX,
                    centerTile.y + offsetY
                };
                if (!visitedCandidateTiles.insert(candidate).second) {
                    continue;
                }

                Rectangle candidateBounds =
                    GetLineOfSightTileBounds(candidate);
                if (!LineOfSightGeometry::CapsuleIntersectsRectangle(
                        start,
                        end,
                        radius,
                        candidateBounds)) {
                    continue;
                }
                if (recordDebug) {
                    debugTrace.candidateTiles.push_back(candidateBounds);
                }

                staticColliders.clear();
                currentLevelProvider->
                    AppendStaticBlockingCollidersForTile(
                        candidate.x,
                        candidate.y,
                        staticColliders
                    );
                for (Rectangle collider : staticColliders) {
                    if (testCollider(collider)) return true;
                }

                auto dynamicEntry = lineOfSightDynamicBlockers.find(
                    candidate
                );
                if (dynamicEntry == lineOfSightDynamicBlockers.end()) {
                    continue;
                }
                for (const MapObject* blocker : dynamicEntry->second) {
                    if (!blocker ||
                        !blocker->IsSolid() ||
                        !testedMapBlockers.insert(blocker).second) {
                        continue;
                    }
                    if (testCollider(blocker->GetCollisionBox())) {
                        return true;
                    }
                }
            }
        }
        return false;
    };

    LineOfSightTile current = WorldToLineOfSightTile(start);
    LineOfSightTile destination = WorldToLineOfSightTile(end);
    if (visitCenterTile(current)) return finishQuery(false);
    if (current == destination) return finishQuery(true);

    float deltaX = end.x - start.x;
    float deltaY = end.y - start.y;
    int stepX = deltaX > 0.0f ? 1 : (deltaX < 0.0f ? -1 : 0);
    int stepY = deltaY > 0.0f ? 1 : (deltaY < 0.0f ? -1 : 0);
    float infinity = std::numeric_limits<float>::infinity();
    float tDeltaX = stepX != 0 ? tileSize / std::abs(deltaX) : infinity;
    float tDeltaY = stepY != 0 ? tileSize / std::abs(deltaY) : infinity;
    Vector2 gridOrigin = GetLineOfSightGridOrigin();
    float nextBoundaryX = gridOrigin.x +
        (stepX > 0 ? current.x + 1 : current.x) * tileSize;
    float nextBoundaryY = gridOrigin.y +
        (stepY > 0 ? current.y + 1 : current.y) * tileSize;
    float tMaxX = stepX != 0
        ? (nextBoundaryX - start.x) / deltaX
        : infinity;
    float tMaxY = stepY != 0
        ? (nextBoundaryY - start.y) / deltaY
        : infinity;

    constexpr float DDA_TIE_EPSILON = 0.000001f;
    int maximumAdvances =
        std::abs(destination.x - current.x) +
        std::abs(destination.y - current.y) + 4;
    int advances = 0;
    while (!(current == destination)) {
        if (++advances > maximumAdvances) return finishQuery(false);

        if (tMaxX + DDA_TIE_EPSILON < tMaxY) {
            current.x += stepX;
            tMaxX += tDeltaX;
            if (visitCenterTile(current)) return finishQuery(false);
            continue;
        }
        if (tMaxY + DDA_TIE_EPSILON < tMaxX) {
            current.y += stepY;
            tMaxY += tDeltaY;
            if (visitCenterTile(current)) return finishQuery(false);
            continue;
        }

        LineOfSightTile sideX = { current.x + stepX, current.y };
        LineOfSightTile sideY = { current.x, current.y + stepY };
        if (stepX != 0 && visitCenterTile(sideX)) {
            return finishQuery(false);
        }
        if (stepY != 0 && visitCenterTile(sideY)) {
            return finishQuery(false);
        }
        current.x += stepX;
        current.y += stepY;
        tMaxX += tDeltaX;
        tMaxY += tDeltaY;
        if (visitCenterTile(current)) return finishQuery(false);
    }

    return finishQuery(true);
}

void LevelManager::DrawLineOfSightDebug() const {
    for (const LineOfSightDebugTrace& trace : lineOfSightDebugTraces) {
        Color resultColor = trace.clear ? LIME : RED;
        for (Rectangle tile : trace.candidateTiles) {
            DrawRectangleLinesEx(tile, 0.5f, Fade(YELLOW, 0.35f));
        }
        for (Rectangle tile : trace.ddaTiles) {
            DrawRectangleLinesEx(tile, 1.0f, Fade(SKYBLUE, 0.8f));
        }
        for (std::size_t index = 0;
             index < trace.testedColliders.size();
             ++index) {
            Rectangle collider = trace.testedColliders[index];
            bool blocking = (int)index == trace.blockingColliderIndex;
            if (blocking) {
                DrawRectangleRec(collider, Fade(RED, 0.3f));
            }
            DrawRectangleLinesEx(
                collider,
                blocking ? 2.0f : 1.0f,
                blocking ? RED : ORANGE
            );
        }

        float diameter = trace.radius * 2.0f;
        if (diameter > 0.0f) {
            DrawLineEx(
                trace.start,
                trace.end,
                diameter,
                Fade(resultColor, 0.15f)
            );
            DrawCircleV(
                trace.start,
                trace.radius,
                Fade(resultColor, 0.15f)
            );
            DrawCircleV(
                trace.end,
                trace.radius,
                Fade(resultColor, 0.15f)
            );
        }
        DrawLineEx(trace.start, trace.end, 1.0f, resultColor);
        DrawCircleV(trace.start, 2.0f, SKYBLUE);
        DrawCircleV(trace.end, 3.0f, resultColor);
    }
}

void LevelManager::DrawMapCollisionDebug() const {
    if (!Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES) return;
    for (const std::unique_ptr<MapObject>& object : mapObjects) {
        if (!object) continue;
        DrawRectangleLinesEx(
            object->GetBoundingBox(),
            Constants::DEBUG_COLLISION_LINE_THICKNESS * 2.0f,
            PURPLE
        );
        DrawRectangleLinesEx(
            object->GetCollisionBox(),
            Constants::DEBUG_COLLISION_LINE_THICKNESS,
            GOLD
        );
    }
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

void LevelManager::ProcessDestroyedMapObjects() {
    bool navigationChanged = false;
    mapObjects.erase(
        std::remove_if(
            mapObjects.begin(),
            mapObjects.end(),
            [this, &navigationChanged](const std::unique_ptr<MapObject>& object) {
                if (!object || !object->IsDestroyed()) return false;

                GameObjectCell cell = object->GetObjectCell();
                if (!IsProceduralDungeon() &&
                    cell.row >= 0 && cell.column >= 0 &&
                    cell.row < (int)mapObjectGrid.size() &&
                    cell.column < (int)mapObjectGrid[cell.row].size()) {
                    mapObjectGrid[cell.row][cell.column] =
                        MapObjectId::Empty;
                }
                navigationChanged = navigationChanged || object->IsSolid();
                return true;
            }
        ),
        mapObjects.end()
    );
    if (navigationChanged) MarkNavigationChanged();
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

DynamicSpawnList LevelManager::GenerateDungeon() {
    DynamicSpawnList dynamicSpawns;
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
    
    levelWidth = activeRoom->width * Constants::RENDER_TILE_SIZE;
    levelHeight = activeRoom->height * Constants::RENDER_TILE_SIZE;

    currentLevelProvider = std::make_unique<ProceduralLevelProvider>(
        activeRoom, roomOffset,
        floorTileset, wallTileset, prop1Texture, prop2Texture,
        boxTexture, gateTexture, levelMap
    );    if (levelMap.spawnRoom) {
        // Auto-discover spawn room and mark it cleared (no combat, pots, or enemies in spawn)
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

                        // Strict guard: Skip prop/pot instantiation if inside the spawn room
                        if (levelMap.spawnRoom && CheckCollisionPointRec(worldPos, levelMap.spawnRoom->GetWorldBounds())) {
                            continue;
                        }

                        if (MapObjectFactory::IsMapObjectType(type)) {
                            AddMapObject(MapObjectFactory::Create(
                                type,
                                worldPos,
                                { y, x }
                            ));
                        } else {
                            dynamicSpawns.push_back({
                                type,
                                worldPos,
                                { y, x }
                            });
                        }
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
                        auto doorObject = std::make_unique<DoorGate>(worldPos);
                        DoorGate* door = doorObject.get();
                        AddMapObject(std::move(doorObject));
                        node->doors.push_back(door);
                    }
                }
            }
            
            // Calculate walkable grid for this room now that all entities (props, doors) are spawned
            node->CalculateWalkableGrid(this);
        }
    }

    printf("GenerateDungeon: Done\n");
    return dynamicSpawns;
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
