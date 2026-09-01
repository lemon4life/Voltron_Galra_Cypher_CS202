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
#include "Core/MissionSaveData.h"

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

// Design Pattern - Bridge abstraction:
// LevelManager owns the active ILevelProvider implementor and delegates map
// rendering/static collision to either StaticLevelProvider or
// ProceduralLevelProvider while retaining shared rooms, props, LOS, and gates.
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
        /// Implements operator for this type.
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
    std::uint64_t checkpointRevision = 0;
    std::unique_ptr<ILevelProvider> currentLevelProvider;
    std::vector<Vector2> staticSpawnNodes;

    bool bossExitGateActive = false;
    Vector2 bossExitGatePosition = {0.0f, 0.0f};

    /// Loads object grid.
    bool LoadObjectGrid(const std::string& filepath);
    /// Spawns map content.
    DynamicSpawnList SpawnMapContent();
    /// Marks navigation changed.
    void MarkNavigationChanged() { ++navigationRevision; }

    /// Processes destroyed map objects.
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
    mutable std::unordered_set<LineOfSightTile, LineOfSightTileHash>
        lineOfSightVisitedCenterTiles;
    mutable std::unordered_set<LineOfSightTile, LineOfSightTileHash>
        lineOfSightVisitedCandidateTiles;
    mutable std::unordered_set<const MapObject*> lineOfSightTestedBlockers;
    mutable std::vector<Rectangle> lineOfSightStaticColliderScratch;

    /// Returns the current line of sight grid origin.
    Vector2 GetLineOfSightGridOrigin() const;
    /// Implements the world to line of sight tile behavior for this component.
    LineOfSightTile WorldToLineOfSightTile(Vector2 position) const;
    /// Returns the current line of sight tile bounds.
    Rectangle GetLineOfSightTileBounds(LineOfSightTile tile) const;
    /// Ensures line of sight blocker index.
    void EnsureLineOfSightBlockerIndex() const;

public:
    /// Creates a LevelManager instance from the supplied configuration.
    LevelManager();
    /// Releases resources owned by this LevelManager instance.
    ~LevelManager();
    /// Initializes assets.
    void InitializeAssets();
    /// Implements the shutdown assets behavior for this component.
    void ShutdownAssets();

    /// Clears the active world, loads a level definition, and creates its runtime objects.
    DynamicSpawnList LoadLevel(const std::string& filepath);

    // Helper for safe spawning
    /// Returns the current safe spawn position.
    bool GetSafeSpawnPosition(std::shared_ptr<RoomNode> room, Vector2& outPos);
    /// Returns the current guaranteed spawn point.
    bool GetGuaranteedSpawnPoint(Vector2& outPos);

    /// Returns the current level map.
    const LevelMap& GetLevelMap() const { return levelMap; }
    /// Returns the current currently locked room.
    std::shared_ptr<RoomNode> GetCurrentlyLockedRoom() const { return currentlyLockedRoom; }
    /// Returns the current active room state.
    RoomState GetActiveRoomState() const { return (currentlyLockedRoom) ? currentlyLockedRoom->state : RoomState::IDLE; }
    /// Updates the stored active room state.
    void SetActiveRoomState(RoomState state);
    /// Reports whether the procedural dungeon condition is satisfied.
    bool IsProceduralDungeon() const {
        return levelMode == LevelMode::Procedural;
    }
    /// Implements the needs player nudge behavior for this component.
    bool NeedsPlayerNudge() const { return needsNudge; }
    /// Consumes and returns nudge.
    Vector2 ConsumeNudge() { needsNudge = false; return nudgePosition; }
    /// Spawns the boss chamber exit gate.
    void SpawnBossExitGate(Vector2 position) {
        bossExitGateActive = true;
        bossExitGatePosition = position;
    }
    /// Reports whether the boss exit gate is active.
    bool IsBossExitGateActive() const { return bossExitGateActive; }
    /// Returns the boss exit gate position.
    Vector2 GetBossExitGatePosition() const { return bossExitGatePosition; }
    /// Reports whether the player in exit room condition is satisfied.
    bool IsPlayerInExitRoom(Vector2 playerPos) const;
    /// Builds the current procedural floor, bakes its rooms into level layers,
    /// creates special-room entities, and exposes all resulting dynamic spawns.
    DynamicSpawnList GenerateDungeon(int floorNumber = 1);

    /// Renders level base.
    void DrawLevelBase();
    /// Returns the current depth render items.
    void GetDepthRenderItems(std::vector<DepthRenderItem>& items);
    /// Advances room discovery, room locking, doors, map objects, and collision state.
    /// The player position is also checked for safe gate nudging during transitions.
    void UpdateLevel(
        float deltaTime,
        Vector2 playerPos = { 0.0f, 0.0f },
        Rectangle playerCollisionBox = {}
    );
    /// Searches for gate escape position.
    bool FindGateEscapePosition(
        Rectangle playerCollisionBox,
        Vector2 playerPosition,
        Vector2& escapePosition
    ) const;
    /// Reports whether the solid collision condition is satisfied.
    bool IsSolidCollision(Rectangle box) const;
    /// Resolves solid movement.
    CollisionMovementResult ResolveSolidMovement(
        Rectangle collisionBox,
        Vector2 desiredDisplacement
    ) const;
    /// Searches for solid map object collision.
    MapObject* FindSolidMapObjectCollision(Rectangle box) const;
    /// Searches for solid map object collisions.
    std::vector<MapObject*> FindSolidMapObjectCollisions(
        Rectangle box
    ) const;
    /// Searches for projectile map object collision.
    MapObject* FindProjectileMapObjectCollision(Rectangle box) const;
    /// Clears level.
    void ClearLevel();
    /// Adds map object.
    MapObject* AddMapObject(std::unique_ptr<MapObject> object);

    /// Implements the world to tile behavior for this component.
    Vector2 WorldToTile(Vector2 worldPos) const;
    /// Implements the tile to world behavior for this component.
    Vector2 TileToWorld(int tileX, int tileY) const;

    /// Returns the current level width.
    float GetLevelWidth() const { return levelWidth; }
    /// Returns the current level height.
    float GetLevelHeight() const { return levelHeight; }
    /// Reports whether this component has clear line of sight.
    bool HasClearLineOfSight(
        Vector2 start,
        Vector2 end,
        float projectileRadius = 5.0f
    ) const override;
    /// Returns the current level bounds.
    Rectangle GetLevelBounds() const;
    /// Renders line of sight debug.
    void DrawLineOfSightDebug() const;
    /// Renders map collision debug.
    void DrawMapCollisionDebug() const;
    /// Returns the current current room bounds.
    Rectangle GetCurrentRoomBounds() const;
    /// Returns the current navigation revision.
    std::uint64_t GetNavigationRevision() const {
        return navigationRevision;
    }
    /// Changes only at stable save points: floor creation, room clear, utility entry.
    std::uint64_t GetCheckpointRevision() const { return checkpointRevision; }
    /// Returns the current map objects.
    const std::vector<std::unique_ptr<MapObject>>& GetMapObjects() const {
        return mapObjects;
    }
    /// Captures the exact generated layout and stable room/map-object progress.
    SavedLevelState CaptureCheckpointState() const;
    /// Rebuilds a generated level from a stable checkpoint.
    bool RestoreCheckpointState(const SavedLevelState& saved);
    /// Returns the current memory stats.
    LevelMemoryStats GetMemoryStats() const;
};
