#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include "raylib.h"
#include "Core/LevelAccess.h"
#include "Core/Utils/CollisionMovement.h"
#include "Core/World/MapObject.h"
#include "Core/World/WorldDefinition.h"
#include "Core/Level/Tilemap.h"
#include "Core/DepthRenderItem.h"
#include "Core/Level/ILevelProvider.h"

struct LevelMemoryStats {
    std::size_t roomNodes = 0;
    std::size_t liveRoomNodes = 0;
    std::size_t mapObjects = 0;
    std::size_t staticSpawnNodes = 0;
    std::size_t layerCells = 0;
    std::size_t lineOfSightBlockerTiles = 0;
    std::size_t lineOfSightTraces = 0;
    std::size_t lineOfSightRectangles = 0;
};

class LevelManager : public ILevelLineOfSightQuery {
private:
    enum class LevelMode {
        Layered,
        Procedural
    };

    struct LineOfSightTile {
        int x;
        int y;

        bool operator==(const LineOfSightTile& other) const {
            return x == other.x && y == other.y;
        }
    };

    struct LineOfSightTileHash {
        std::size_t operator()(const LineOfSightTile& tile) const {
            std::size_t xHash = std::hash<int>()(tile.x);
            std::size_t yHash = std::hash<int>()(tile.y);
            return xHash ^ (yHash << 1);
        }
    };

    struct LineOfSightDebugTrace {
        Vector2 start = { 0.0f, 0.0f };
        Vector2 end = { 0.0f, 0.0f };
        float radius = 0.0f;
        bool clear = false;
        std::vector<Rectangle> ddaTiles;
        std::vector<Rectangle> candidateTiles;
        std::vector<Rectangle> testedColliders;
        int blockingColliderIndex = -1;
    };

    std::vector<std::unique_ptr<MapObject>> mapObjects;
    float levelWidth;
    float levelHeight;
    int gridRows;
    int gridCols;
    std::vector<std::vector<int>> mapGridLayer1;
    std::vector<std::vector<int>> mapGridLayer2;
    std::vector<std::vector<MapObjectId>> mapObjectGrid;
    Texture2D floorTileset = {};
    Texture2D wallTileset = {};
    Texture2D boxTexture = {};
    Texture2D gateTexture = {};
    Texture2D prop1Texture = {};
    Texture2D prop2Texture = {};
    LevelMode levelMode = LevelMode::Layered;
    std::shared_ptr<RoomTemplate> activeRoom;
    LevelMap levelMap;
    std::shared_ptr<RoomNode> currentlyLockedRoom;
    Vector2 roomOffset = {0.0f, 0.0f};
    Vector2 nudgePosition = {0.0f, 0.0f};
    bool needsNudge = false;
    std::uint64_t navigationRevision = 0;
    std::unique_ptr<ILevelProvider> currentLevelProvider;
    std::vector<Vector2> staticSpawnNodes;

    bool LoadObjectGrid(const std::string& filepath);
    DynamicSpawnList SpawnMapContent();
    void MarkNavigationChanged() { ++navigationRevision; }

    void ProcessDestroyedMapObjects();
    mutable bool lineOfSightBlockerIndexValid = false;
    mutable std::uint64_t lineOfSightBlockerIndexRevision = 0;
    mutable Vector2 lineOfSightBlockerIndexOrigin = { 0.0f, 0.0f };
    mutable const RoomTemplate* lineOfSightBlockerIndexRoom = nullptr;
    mutable std::unordered_map<
        LineOfSightTile,
        std::vector<MapObject*>,
        LineOfSightTileHash
    > lineOfSightDynamicBlockers;
    mutable std::vector<LineOfSightDebugTrace> lineOfSightDebugTraces;

    Vector2 GetLineOfSightGridOrigin() const;
    LineOfSightTile WorldToLineOfSightTile(Vector2 position) const;
    Rectangle GetLineOfSightTileBounds(LineOfSightTile tile) const;
    void EnsureLineOfSightBlockerIndex() const;

public:
    LevelManager();
    ~LevelManager();
    void InitializeAssets();
    void ShutdownAssets();

    DynamicSpawnList LoadLevel(const std::string& filepath);

    // Helper for safe spawning
    bool GetSafeSpawnPosition(std::shared_ptr<RoomNode> room, Vector2& outPos);
    bool GetGuaranteedSpawnPoint(Vector2& outPos);

    const LevelMap& GetLevelMap() const { return levelMap; }
    std::shared_ptr<RoomNode> GetCurrentlyLockedRoom() const { return currentlyLockedRoom; }
    RoomState GetActiveRoomState() const { return (currentlyLockedRoom) ? currentlyLockedRoom->state : RoomState::IDLE; }
    void SetActiveRoomState(RoomState state);
    bool IsProceduralDungeon() const {
        return levelMode == LevelMode::Procedural;
    }
    bool NeedsPlayerNudge() const { return needsNudge; }
    Vector2 ConsumeNudge() { needsNudge = false; return nudgePosition; }
    bool IsPlayerInExitRoom(Vector2 playerPos) const;
    DynamicSpawnList GenerateDungeon(int floorNumber = 1);

    void DrawLevelBase();
    void GetDepthRenderItems(std::vector<DepthRenderItem>& items);
    void UpdateLevel(
        float deltaTime,
        Vector2 playerPos = { 0.0f, 0.0f },
        Rectangle playerCollisionBox = {}
    );
    bool FindGateEscapePosition(
        Rectangle playerCollisionBox,
        Vector2 playerPosition,
        Vector2& escapePosition
    ) const;
    bool IsSolidCollision(Rectangle box) const;
    CollisionMovementResult ResolveSolidMovement(
        Rectangle collisionBox,
        Vector2 desiredDisplacement
    ) const;
    MapObject* FindSolidMapObjectCollision(Rectangle box) const;
    std::vector<MapObject*> FindSolidMapObjectCollisions(
        Rectangle box
    ) const;
    MapObject* FindProjectileMapObjectCollision(Rectangle box) const;
    void ClearLevel();
    MapObject* AddMapObject(std::unique_ptr<MapObject> object);

    Vector2 WorldToTile(Vector2 worldPos) const;
    Vector2 TileToWorld(int tileX, int tileY) const;

    float GetLevelWidth() const { return levelWidth; }
    float GetLevelHeight() const { return levelHeight; }
    bool HasClearLineOfSight(
        Vector2 start,
        Vector2 end,
        float projectileRadius = 5.0f
    ) const override;
    Rectangle GetLevelBounds() const;
    void DrawLineOfSightDebug() const;
    void DrawMapCollisionDebug() const;
    Rectangle GetCurrentRoomBounds() const;
    std::uint64_t GetNavigationRevision() const {
        return navigationRevision;
    }
    const std::vector<std::unique_ptr<MapObject>>& GetMapObjects() const {
        return mapObjects;
    }
    LevelMemoryStats GetMemoryStats() const;
};
