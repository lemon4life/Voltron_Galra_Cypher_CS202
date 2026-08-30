#include "Core/Manager/PathFindingManager.h"
#include "Core/Manager/ObjectManager.h"

#include "Core/Constants.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"

#include "raymath.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <limits>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct NavigationCacheKey {
    int width;
    int height;
    int offsetX;
    int offsetY;
    int minimumTileX;
    int minimumTileY;
    int tileWidth;
    int tileHeight;

    bool operator==(const NavigationCacheKey& other) const {
        return width == other.width && height == other.height &&
            offsetX == other.offsetX && offsetY == other.offsetY &&
            minimumTileX == other.minimumTileX &&
            minimumTileY == other.minimumTileY &&
            tileWidth == other.tileWidth && tileHeight == other.tileHeight;
    }
};

struct NavigationCacheKeyHash {
    /// Implements operator for this type.
    std::size_t operator()(const NavigationCacheKey& key) const {
        std::size_t value = 0;
        auto combine = [&value](int component) {
            value ^= std::hash<int>()(component) + 0x9e3779b9U +
                (value << 6) + (value >> 2);
        };
        combine(key.width);
        combine(key.height);
        combine(key.offsetX);
        combine(key.offsetY);
        combine(key.minimumTileX);
        combine(key.minimumTileY);
        combine(key.tileWidth);
        combine(key.tileHeight);
        return value;
    }
};

struct NavigationGridCache {
    Rectangle searchBounds = {};
    int minimumTileX = 0;
    int minimumTileY = 0;
    int tileWidth = 0;
    int tileHeight = 0;
    std::vector<std::int8_t> clearTiles;
    std::vector<std::array<std::int8_t, 8>> clearEdges;
};

struct FlowFieldCell {
    float integrationCost = std::numeric_limits<float>::max();
    int nextCellIndex = -1;
    int terminalGoalIndex = -1;
    bool reachable = false;
};

struct FlowFieldTerminal {
    int tileX = 0;
    int tileY = 0;
    Vector2 goalPosition = { 0.0f, 0.0f };
    std::vector<Vector2> suffix;
    float connectionDistance = 0.0f;
};

struct FlowFieldData {
    NavigationCacheKey key = {};
    Rectangle searchBounds = {};
    std::vector<FlowFieldCell> cells;
    std::vector<FlowFieldTerminal> terminals;
    int expandedCells = 0;
    bool available = false;
    bool oversized = false;
};

struct EnemyNavigationCacheStore {
    std::uint64_t navigationRevision =
        std::numeric_limits<std::uint64_t>::max();
    std::unordered_map<
        NavigationCacheKey,
        NavigationGridCache,
        NavigationCacheKeyHash
    > grids;
    bool goalsValid = false;
    int goalTargetTileX = 0;
    int goalTargetTileY = 0;
    Rectangle goalSearchBounds = {};
    Rectangle goalTargetBounds = {};
    const Paladin* goalTarget = nullptr;
    std::uint64_t goalRevision = 0;
    std::vector<EnemyPathDebugPoint> sharedGoals;
    std::unordered_map<
        NavigationCacheKey,
        FlowFieldData,
        NavigationCacheKeyHash
    > flowFields;

    /// Ensures revision.
    void EnsureRevision(std::uint64_t revision) {
        if (navigationRevision == revision) return;
        navigationRevision = revision;
        grids.clear();
        goalsValid = false;
        goalTarget = nullptr;
        goalTargetBounds = {};
        sharedGoals.clear();
        flowFields.clear();
    }
};

namespace {
    constexpr float DIAGONAL_COST = 1.41421356f;
    constexpr float TARGET_LOOP_ALL_INTERVAL = 0.6f;
    constexpr float BODY_PATH_SAMPLE_SPACING = 4.0f;
    constexpr float POSITION_EPSILON_SQUARED = 4.0f;
    constexpr float QUARTER_TILE_OFFSET = 8.0f;
    constexpr int MAX_SEARCH_STEPS = 70;
    constexpr int MAX_FLOW_FIELD_CELLS = 4096;
    constexpr int MAX_TARGET_POSITIONS = 7;
    constexpr int MAX_SEARCHES_PER_FRAME = 1;
    constexpr int MAX_START_CONNECTION_ATTEMPTS = 32;
    constexpr float FAILED_PATH_RETRY_INTERVAL = 1.2f;
    constexpr float GOAL_LINE_OF_SIGHT_CORNER_MARGIN = 0.5f;

    struct Tile {
        int x;
        int y;

        bool operator==(const Tile& other) const {
            return x == other.x && y == other.y;
        }
    };

    struct TileHash {
        /// Implements operator for this type.
        std::size_t operator()(const Tile& tile) const {
            std::size_t xHash = std::hash<int>()(tile.x);
            std::size_t yHash = std::hash<int>()(tile.y);
            return xHash ^ (yHash << 1);
        }
    };

    struct AStarNode {
        Tile tile;
        float gCost;
        float fCost;
    };

    struct CompareAStarNode {
        /// Implements operator for this type.
        bool operator()(const AStarNode& first, const AStarNode& second) const {
            return first.fCost > second.fCost;
        }
    };

    struct PositionConnection {
        Tile graphTile;
        std::vector<Vector2> waypoints;
        float distance = std::numeric_limits<float>::max();
    };

    struct GoalAnchor {
        Tile graphTile;
        Vector2 goalPosition;
        std::vector<Vector2> suffix;
        float connectionDistance;
    };

    struct PathSearchResult {
        EnemyPathStatus status = EnemyPathStatus::Unreachable;
        std::vector<Vector2> waypoints;
        std::vector<EnemyPathDebugPoint> debugGoals;
        std::optional<Vector2> selectedGoal;
        int expandedCells = 0;
    };

    /// Implements the distance squared behavior for this component.
    float DistanceSquared(Vector2 first, Vector2 second) {
        float dx = first.x - second.x;
        float dy = first.y - second.y;
        return dx * dx + dy * dy;
    }

    /// Implements the almost same position behavior for this component.
    bool AlmostSamePosition(Vector2 first, Vector2 second) {
        return DistanceSquared(first, second) < POSITION_EPSILON_SQUARED;
    }

    /// Closes st point on rectangle.
    Vector2 ClosestPointOnRectangle(Vector2 point, Rectangle rectangle) {
        return {
            std::clamp(
                point.x,
                rectangle.x,
                rectangle.x + rectangle.width
            ),
            std::clamp(
                point.y,
                rectangle.y,
                rectangle.y + rectangle.height
            )
        };
    }

    /// Implements the connection length behavior for this component.
    float ConnectionLength(Vector2 start, const std::vector<Vector2>& points) {
        float length = 0.0f;
        Vector2 previous = start;
        for (Vector2 point : points) {
            length += Vector2Distance(previous, point);
            previous = point;
        }
        return length;
    }

    /// Implements the heuristic behavior for this component.
    float Heuristic(Tile start, Tile goal) {
        int dx = std::abs(start.x - goal.x);
        int dy = std::abs(start.y - goal.y);
        int straightSteps = std::abs(dx - dy);
        int diagonalSteps = std::min(dx, dy);
        return (float)straightSteps + DIAGONAL_COST * (float)diagonalSteps;
    }

    /// Implements the heuristic to goals behavior for this component.
    float HeuristicToGoals(Tile tile, const std::vector<GoalAnchor>& goals) {
        float best = std::numeric_limits<float>::max();
        for (const GoalAnchor& goal : goals) {
            best = std::min(best, Heuristic(tile, goal.graphTile));
        }
        return best;
    }

    /// Reports whether the inner rectangle fits completely inside the outer rectangle.
    bool ContainsRectangle(Rectangle outer, Rectangle inner) {
        constexpr float EDGE_PADDING = 0.001f;
        return inner.x >= outer.x - EDGE_PADDING &&
            inner.y >= outer.y - EDGE_PADDING &&
            inner.x + inner.width <= outer.x + outer.width + EDGE_PADDING &&
            inner.y + inner.height <= outer.y + outer.height + EDGE_PADDING;
    }

    /// Implements the navigation center at entity position behavior for this component.
    Vector2 NavigationCenterAtEntityPosition(
        const Enemy& enemy,
        Vector2 entityPosition
    ) {
        Vector2 offset = enemy.GetCollisionProfile().navigationCenterOffset;
        return Vector2Add(entityPosition, offset);
    }

    /// Calculates and returns entity position at navigation center.
    Vector2 EntityPositionAtNavigationCenter(
        const Enemy& enemy,
        Vector2 navigationCenter
    ) {
        Vector2 offset = enemy.GetCollisionProfile().navigationCenterOffset;
        return Vector2Subtract(navigationCenter, offset);
    }

    /// Calculates and returns navigation footprint at center.
    Rectangle NavigationFootprintAtCenter(
        const Enemy& enemy,
        Vector2 navigationCenter
    ) {
        Vector2 size = enemy.GetCollisionProfile().navigationSize;
        return {
            navigationCenter.x - size.x * 0.5f,
            navigationCenter.y - size.y * 0.5f,
            size.x,
            size.y
        };
    }

    /// Reports whether the body clear at navigation center condition is satisfied.
    bool IsBodyClearAtNavigationCenter(
        const LevelManager& levelManager,
        const Enemy& enemy,
        Vector2 navigationCenter,
        Rectangle searchBounds
    ) {
        Rectangle footprint = NavigationFootprintAtCenter(
            enemy,
            navigationCenter
        );
        return ContainsRectangle(searchBounds, footprint) &&
            !levelManager.IsSolidCollision(footprint);
    }

    /// Reports whether the navigation path clear condition is satisfied.
    bool IsNavigationPathClear(
        const LevelManager& levelManager,
        const Enemy& enemy,
        Vector2 start,
        Vector2 end,
        Rectangle searchBounds
    ) {
        float distance = Vector2Distance(start, end);
        int sampleCount = std::max(
            1,
            (int)std::ceil(distance / BODY_PATH_SAMPLE_SPACING)
        );

        for (int sample = 0; sample <= sampleCount; ++sample) {
            float amount = (float)sample / (float)sampleCount;
            Vector2 navigationCenter = Vector2Lerp(start, end, amount);
            if (!IsBodyClearAtNavigationCenter(
                    levelManager,
                    enemy,
                    navigationCenter,
                    searchBounds)) {
                return false;
            }
        }

        return true;
    }

    /// Reports whether the body clear at world position condition is satisfied.
    bool IsBodyClearAtWorldPosition(
        const LevelManager& levelManager,
        const Enemy& enemy,
        Vector2 entityPosition,
        Rectangle searchBounds
    ) {
        return IsBodyClearAtNavigationCenter(
            levelManager,
            enemy,
            NavigationCenterAtEntityPosition(enemy, entityPosition),
            searchBounds
        );
    }

    /// Reports whether the body path clear condition is satisfied.
    bool IsBodyPathClear(
        const LevelManager& levelManager,
        const Enemy& enemy,
        Vector2 entityStart,
        Vector2 entityEnd,
        Rectangle searchBounds
    ) {
        return IsNavigationPathClear(
            levelManager,
            enemy,
            NavigationCenterAtEntityPosition(enemy, entityStart),
            NavigationCenterAtEntityPosition(enemy, entityEnd),
            searchBounds
        );
    }

    /// Implements the connect positions behavior for this component.
    std::optional<std::vector<Vector2>> ConnectPositions(
        const LevelManager& levelManager,
        const Enemy& enemy,
        Vector2 start,
        Vector2 end,
        Rectangle searchBounds
    ) {
        if (IsNavigationPathClear(
                levelManager,
                enemy,
                start,
                end,
                searchBounds)) {
            return std::vector<Vector2>{ end };
        }

        std::array<Vector2, 2> corners = {
            Vector2{ end.x, start.y },
            Vector2{ start.x, end.y }
        };
        std::optional<std::vector<Vector2>> best;
        float bestLength = std::numeric_limits<float>::max();

        for (Vector2 corner : corners) {
            if (!IsBodyClearAtNavigationCenter(
                    levelManager,
                    enemy,
                    corner,
                    searchBounds) ||
                !IsNavigationPathClear(
                    levelManager,
                    enemy,
                    start,
                    corner,
                    searchBounds) ||
                !IsNavigationPathClear(
                    levelManager,
                    enemy,
                    corner,
                    end,
                    searchBounds)) {
                continue;
            }

            std::vector<Vector2> points;
            if (!AlmostSamePosition(start, corner)) {
                points.push_back(corner);
            }
            if (points.empty() || !AlmostSamePosition(points.back(), end)) {
                points.push_back(end);
            }

            float length = ConnectionLength(start, points);
            if (length < bestLength) {
                bestLength = length;
                best = std::move(points);
            }
        }

        return best;
    }

    class SearchCollisionCache {
    public:
        /// Creates a SearchCollisionCache instance from the supplied configuration.
        SearchCollisionCache(
            LevelManager& levelManager,
            Enemy& enemy,
            EnemyNavigationCacheStore& store,
            Rectangle searchBounds,
            std::uint64_t navigationRevision
        )
            : levelManager(levelManager),
              enemy(enemy),
              searchBounds(searchBounds) {
            store.EnsureRevision(navigationRevision);

            EnemyCollisionProfile profile = enemy.GetCollisionProfile();
            int minimumTileX = (int)std::floor(
                searchBounds.x / Constants::RENDER_TILE_SIZE
            );
            int minimumTileY = (int)std::floor(
                searchBounds.y / Constants::RENDER_TILE_SIZE
            );
            int maximumTileX = (int)std::ceil(
                (searchBounds.x + searchBounds.width) /
                    Constants::RENDER_TILE_SIZE
            ) - 1;
            int maximumTileY = (int)std::ceil(
                (searchBounds.y + searchBounds.height) /
                    Constants::RENDER_TILE_SIZE
            ) - 1;

            key = {
                Quantize(profile.navigationSize.x),
                Quantize(profile.navigationSize.y),
                Quantize(profile.navigationCenterOffset.x),
                Quantize(profile.navigationCenterOffset.y),
                minimumTileX,
                minimumTileY,
                std::max(0, maximumTileX - minimumTileX + 1),
                std::max(0, maximumTileY - minimumTileY + 1)
            };

            auto existing = store.grids.find(key);
            if (existing == store.grids.end()) {
                NavigationGridCache created;
                created.searchBounds = searchBounds;
                created.minimumTileX = key.minimumTileX;
                created.minimumTileY = key.minimumTileY;
                created.tileWidth = key.tileWidth;
                created.tileHeight = key.tileHeight;
                int cellCount = key.tileWidth * key.tileHeight;
                created.clearTiles.assign(cellCount, (std::int8_t)-1);
                created.clearEdges.resize(cellCount);
                for (auto& edges : created.clearEdges) {
                    edges.fill((std::int8_t)-1);
                }
                existing = store.grids.emplace(
                    key,
                    std::move(created)
                ).first;
            }
            grid = &existing->second;
        }

        /// Reports whether the tile clear condition is satisfied.
        bool IsTileClear(Tile tile) {
            int index = GetIndex(tile);
            if (index < 0) return false;
            std::int8_t& cached = grid->clearTiles[index];
            if (cached >= 0) return cached != 0;

            bool clear = IsBodyClearAtNavigationCenter(
                levelManager,
                enemy,
                levelManager.TileToWorld(tile.x, tile.y),
                searchBounds
            );
            cached = clear ? 1 : 0;
            return clear;
        }

        /// Reports whether the edge clear condition is satisfied.
        bool IsEdgeClear(Tile current, Tile neighbor) {
            int currentIndex = GetIndex(current);
            int neighborIndex = GetIndex(neighbor);
            int direction = DirectionIndex(
                neighbor.x - current.x,
                neighbor.y - current.y
            );
            if (currentIndex < 0 || neighborIndex < 0 || direction < 0) {
                return false;
            }

            std::int8_t& cached = grid->clearEdges[currentIndex][direction];
            if (cached >= 0) return cached != 0;

            bool clear = CalculateEdgeClear(current, neighbor);
            cached = clear ? 1 : 0;
            int reverseDirection = DirectionIndex(
                current.x - neighbor.x,
                current.y - neighbor.y
            );
            if (reverseDirection >= 0) {
                grid->clearEdges[neighborIndex][reverseDirection] = cached;
            }
            return clear;
        }

        /// Returns the current key.
        const NavigationCacheKey& GetKey() const {
            return key;
        }

        /// Returns the current index.
        int GetIndex(Tile tile) const {
            int localX = tile.x - grid->minimumTileX;
            int localY = tile.y - grid->minimumTileY;
            if (localX < 0 || localY < 0 ||
                localX >= grid->tileWidth || localY >= grid->tileHeight) {
                return -1;
            }
            return localY * grid->tileWidth + localX;
        }

        /// Returns the current tile.
        Tile GetTile(int index) const {
            if (index < 0 || index >= GetCellCount()) return { 0, 0 };
            return {
                grid->minimumTileX + index % grid->tileWidth,
                grid->minimumTileY + index / grid->tileWidth
            };
        }

        /// Returns the current cell count.
        int GetCellCount() const {
            return grid->tileWidth * grid->tileHeight;
        }

    private:
        /// Implements the quantize behavior for this component.
        static int Quantize(float value) {
            return (int)std::lround(value * 1000.0f);
        }

        /// Implements the direction index behavior for this component.
        static int DirectionIndex(int dx, int dy) {
            constexpr std::array<Tile, 8> DIRECTIONS = {
                Tile{ 1, 0 }, Tile{ -1, 0 }, Tile{ 0, 1 }, Tile{ 0, -1 },
                Tile{ 1, 1 }, Tile{ 1, -1 }, Tile{ -1, 1 }, Tile{ -1, -1 }
            };
            for (int index = 0; index < (int)DIRECTIONS.size(); ++index) {
                if (DIRECTIONS[index].x == dx && DIRECTIONS[index].y == dy) {
                    return index;
                }
            }
            return -1;
        }

        /// Calculates edge clear.
        bool CalculateEdgeClear(Tile current, Tile neighbor) {
            if (!IsTileClear(neighbor)) return false;

            int dx = neighbor.x - current.x;
            int dy = neighbor.y - current.y;
            if (dx != 0 && dy != 0) {
                if (!IsTileClear({ current.x + dx, current.y }) ||
                    !IsTileClear({ current.x, current.y + dy })) {
                    return false;
                }
            }

            return IsNavigationPathClear(
                levelManager,
                enemy,
                levelManager.TileToWorld(current.x, current.y),
                levelManager.TileToWorld(neighbor.x, neighbor.y),
                searchBounds
            );
        }

        LevelManager& levelManager;
        Enemy& enemy;
        Rectangle searchBounds;
        NavigationCacheKey key = {};
        NavigationGridCache* grid = nullptr;
    };

    /// Implements the world tile behavior for this component.
    Tile WorldTile(const LevelManager& levelManager, Vector2 position) {
        Vector2 tile = levelManager.WorldToTile(position);
        return { (int)tile.x, (int)tile.y };
    }

    /// Implements the nearby tiles behavior for this component.
    std::vector<Tile> NearbyTiles(Tile center) {
        std::vector<Tile> tiles;
        tiles.reserve(9);
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                tiles.push_back({ center.x + x, center.y + y });
            }
        }
        return tiles;
    }

    /// Searches for start connection.
    std::optional<PositionConnection> FindStartConnection(
        LevelManager& levelManager,
        Enemy& enemy,
        SearchCollisionCache& cache,
        Rectangle searchBounds
    ) {
        Vector2 start = NavigationCenterAtEntityPosition(
            enemy,
            enemy.GetPosition()
        );
        Tile startTile = WorldTile(levelManager, start);
        std::optional<PositionConnection> best;

        auto consider = [&](Tile graphTile, std::vector<Vector2> points) {
            float distance = ConnectionLength(start, points);
            if (!best || distance < best->distance) {
                best = PositionConnection{
                    graphTile,
                    std::move(points),
                    distance
                };
            }
        };

        std::vector<Tile> graphTiles = NearbyTiles(startTile);
        std::sort(
            graphTiles.begin(),
            graphTiles.end(),
            [&](Tile first, Tile second) {
                return DistanceSquared(
                           start,
                           levelManager.TileToWorld(first.x, first.y)) <
                    DistanceSquared(
                           start,
                           levelManager.TileToWorld(second.x, second.y));
            }
        );
        for (Tile graphTile : graphTiles) {
            if (!cache.IsTileClear(graphTile)) continue;
            Vector2 center = levelManager.TileToWorld(graphTile.x, graphTile.y);
            auto connection = ConnectPositions(
                levelManager,
                enemy,
                start,
                center,
                searchBounds
            );
            if (connection) {
                consider(graphTile, std::move(*connection));
            }
        }

        if (best) return best;

        constexpr std::array<Vector2, 8> ANCHOR_OFFSETS = {
            Vector2{ QUARTER_TILE_OFFSET, 0.0f },
            Vector2{ -QUARTER_TILE_OFFSET, 0.0f },
            Vector2{ 0.0f, QUARTER_TILE_OFFSET },
            Vector2{ 0.0f, -QUARTER_TILE_OFFSET },
            Vector2{ QUARTER_TILE_OFFSET, QUARTER_TILE_OFFSET },
            Vector2{ QUARTER_TILE_OFFSET, -QUARTER_TILE_OFFSET },
            Vector2{ -QUARTER_TILE_OFFSET, QUARTER_TILE_OFFSET },
            Vector2{ -QUARTER_TILE_OFFSET, -QUARTER_TILE_OFFSET }
        };

        int connectionAttempts = 0;
        for (Tile anchorTile : graphTiles) {
            if (!cache.IsTileClear(anchorTile)) continue;
            Vector2 tileCenter = levelManager.TileToWorld(
                anchorTile.x,
                anchorTile.y
            );
            for (Vector2 offset : ANCHOR_OFFSETS) {
                if (connectionAttempts >= MAX_START_CONNECTION_ATTEMPTS) {
                    return best;
                }
                ++connectionAttempts;

                Vector2 anchor = Vector2Add(tileCenter, offset);
                if (!IsBodyClearAtNavigationCenter(
                        levelManager,
                        enemy,
                        anchor,
                        searchBounds)) {
                    continue;
                }

                auto toAnchor = ConnectPositions(
                    levelManager,
                    enemy,
                    start,
                    anchor,
                    searchBounds
                );
                if (!toAnchor) continue;

                auto toGraph = ConnectPositions(
                    levelManager,
                    enemy,
                    anchor,
                    tileCenter,
                    searchBounds
                );
                if (!toGraph) continue;

                std::vector<Vector2> points = *toAnchor;
                points.insert(points.end(), toGraph->begin(), toGraph->end());
                consider(anchorTile, std::move(points));
            }
        }

        return best;
    }

    /// Plays er collision tile.
    Tile PlayerCollisionTile(
        const LevelManager& levelManager,
        const Paladin& target
    ) {
        Rectangle collisionBox = target.GetCollisionBox();
        Vector2 collisionCenter = {
            collisionBox.x + collisionBox.width * 0.5f,
            collisionBox.y + collisionBox.height * 0.5f
        };
        return WorldTile(levelManager, collisionCenter);
    }

    /// Implements the generate goal candidates behavior for this component.
    std::vector<Vector2> GenerateGoalCandidates(
        const LevelManager& levelManager,
        const Paladin& target
    ) {
        constexpr std::array<Tile, 8> NEIGHBOR_OFFSETS = {
            Tile{ -1, -1 }, Tile{ 0, -1 }, Tile{ 1, -1 },
            Tile{ -1, 0 },                   Tile{ 1, 0 },
            Tile{ -1, 1 },  Tile{ 0, 1 },   Tile{ 1, 1 }
        };

        std::vector<Vector2> candidates;
        candidates.reserve(NEIGHBOR_OFFSETS.size());
        Tile playerTile = PlayerCollisionTile(levelManager, target);

        for (Tile offset : NEIGHBOR_OFFSETS) {
            candidates.push_back(levelManager.TileToWorld(
                playerTile.x + offset.x,
                playerTile.y + offset.y
            ));
        }

        return candidates;
    }

    /// Implements the same rectangle behavior for this component.
    bool SameRectangle(Rectangle first, Rectangle second) {
        return std::abs(first.x - second.x) < 0.001f &&
            std::abs(first.y - second.y) < 0.001f &&
            std::abs(first.width - second.width) < 0.001f &&
            std::abs(first.height - second.height) < 0.001f;
    }

    /// Returns the current shared goal candidates.
    const std::vector<EnemyPathDebugPoint>& GetSharedGoalCandidates(
        LevelManager& levelManager,
        const Paladin& target,
        EnemyNavigationCacheStore& store,
        Rectangle searchBounds
    ) {
        Tile targetTile = PlayerCollisionTile(levelManager, target);
        Rectangle targetBounds = target.GetBoundingBox();
        bool sameGoalLayout = store.goalsValid &&
            store.goalTarget == &target &&
            store.goalTargetTileX == targetTile.x &&
            store.goalTargetTileY == targetTile.y &&
            SameRectangle(store.goalSearchBounds, searchBounds);
        if (sameGoalLayout &&
            SameRectangle(store.goalTargetBounds, targetBounds)) {
            return store.sharedGoals;
        }

        std::vector<Vector2> candidates;
        if (sameGoalLayout) {
            candidates.reserve(store.sharedGoals.size());
            for (const EnemyPathDebugPoint& goal : store.sharedGoals) {
                candidates.push_back(goal.position);
            }
        } else {
            candidates = GenerateGoalCandidates(levelManager, target);
        }

        std::vector<EnemyPathDebugPoint> refreshedGoals;
        refreshedGoals.reserve(candidates.size());
        Vector2 targetTileCenter = levelManager.TileToWorld(
            targetTile.x,
            targetTile.y
        );
        bool targetTileInsideDomain = CheckCollisionPointRec(
            targetTileCenter,
            searchBounds
        );
        for (Vector2 candidate : candidates) {
            Vector2 closestHitboxPoint = ClosestPointOnRectangle(
                candidate,
                targetBounds
            );
            bool insideDomain = CheckCollisionPointRec(
                candidate,
                searchBounds
            );
            bool hasClearTileConnection = insideDomain &&
                targetTileInsideDomain &&
                levelManager.HasClearLineOfSight(
                    candidate,
                    targetTileCenter,
                    GOAL_LINE_OF_SIGHT_CORNER_MARGIN
                );
            bool hasLineOfSight = hasClearTileConnection &&
                levelManager.HasClearLineOfSight(
                    candidate,
                    closestHitboxPoint,
                    GOAL_LINE_OF_SIGHT_CORNER_MARGIN
                );
            refreshedGoals.push_back({ candidate, hasLineOfSight });
        }

        bool availabilityChanged = !sameGoalLayout ||
            refreshedGoals.size() != store.sharedGoals.size();
        if (!availabilityChanged) {
            for (std::size_t index = 0;
                 index < refreshedGoals.size();
                 ++index) {
                if (refreshedGoals[index].hasLineOfSight !=
                    store.sharedGoals[index].hasLineOfSight) {
                    availabilityChanged = true;
                    break;
                }
            }
        }

        store.goalsValid = true;
        store.goalTarget = &target;
        store.goalTargetTileX = targetTile.x;
        store.goalTargetTileY = targetTile.y;
        store.goalSearchBounds = searchBounds;
        store.goalTargetBounds = targetBounds;
        store.sharedGoals = std::move(refreshedGoals);
        if (availabilityChanged) {
            ++store.goalRevision;
            store.flowFields.clear();
        }
        return store.sharedGoals;
    }

    /// Builds goal anchors.
    std::vector<GoalAnchor> BuildGoalAnchors(
        LevelManager& levelManager,
        Enemy& enemy,
        const std::vector<EnemyPathDebugPoint>& candidates,
        SearchCollisionCache& cache,
        std::vector<EnemyPathDebugPoint>& debugGoals,
        Rectangle searchBounds
    ) {
        std::vector<GoalAnchor> anchors;
        debugGoals = candidates;

        for (const EnemyPathDebugPoint& candidateData : candidates) {
            Vector2 candidate = candidateData.position;
            if (!candidateData.hasLineOfSight) continue;

            Tile candidateTile = WorldTile(levelManager, candidate);
            for (Tile graphTile : NearbyTiles(candidateTile)) {
                if (!cache.IsTileClear(graphTile)) continue;

                Vector2 graphCenter = levelManager.TileToWorld(
                    graphTile.x,
                    graphTile.y
                );
                auto connection = ConnectPositions(
                    levelManager,
                    enemy,
                    graphCenter,
                    candidate,
                    searchBounds
                );
                if (!connection) continue;

                anchors.push_back({
                    graphTile,
                    candidate,
                    *connection,
                    ConnectionLength(graphCenter, *connection)
                });
            }
        }

        return anchors;
    }

    /// Searches for goal at tile.
    const GoalAnchor* FindGoalAtTile(
        Tile tile,
        const std::vector<GoalAnchor>& goals
    ) {
        const GoalAnchor* best = nullptr;
        for (const GoalAnchor& goal : goals) {
            if (!(goal.graphTile == tile)) continue;
            if (!best || goal.connectionDistance < best->connectionDistance) {
                best = &goal;
            }
        }
        return best;
    }

    struct FlowQueueNode {
        int cellIndex;
        float cost;
    };

    struct CompareFlowQueueNode {
        /// Implements operator for this type.
        bool operator()(
            const FlowQueueNode& first,
            const FlowQueueNode& second
        ) const {
            return first.cost > second.cost;
        }
    };

    /// Builds flow field.
    FlowFieldData BuildFlowField(
        LevelManager& levelManager,
        Enemy& representative,
        const std::vector<EnemyPathDebugPoint>& candidates,
        EnemyNavigationCacheStore& store,
        Rectangle searchBounds,
        std::uint64_t navigationRevision
    ) {
        SearchCollisionCache cache(
            levelManager,
            representative,
            store,
            searchBounds,
            navigationRevision
        );

        FlowFieldData field;
        field.key = cache.GetKey();
        field.searchBounds = searchBounds;
        int cellCount = cache.GetCellCount();
        if (cellCount <= 0 || cellCount > MAX_FLOW_FIELD_CELLS) {
            field.oversized = cellCount > MAX_FLOW_FIELD_CELLS;
            return field;
        }

        std::vector<EnemyPathDebugPoint> ignoredDebugGoals;
        std::vector<GoalAnchor> anchors = BuildGoalAnchors(
            levelManager,
            representative,
            candidates,
            cache,
            ignoredDebugGoals,
            searchBounds
        );
        if (anchors.empty()) return field;

        field.cells.resize(cellCount);
        field.terminals.reserve(anchors.size());
        std::priority_queue<
            FlowQueueNode,
            std::vector<FlowQueueNode>,
            CompareFlowQueueNode
        > openSet;

        float tileSize = Constants::RENDER_TILE_SIZE;
        for (const GoalAnchor& anchor : anchors) {
            int cellIndex = cache.GetIndex(anchor.graphTile);
            if (cellIndex < 0) continue;

            int terminalIndex = (int)field.terminals.size();
            field.terminals.push_back({
                anchor.graphTile.x,
                anchor.graphTile.y,
                anchor.goalPosition,
                anchor.suffix,
                anchor.connectionDistance
            });

            float initialCost = anchor.connectionDistance / tileSize;
            FlowFieldCell& cell = field.cells[cellIndex];
            if (cell.reachable && initialCost >= cell.integrationCost) {
                continue;
            }
            cell.integrationCost = initialCost;
            cell.nextCellIndex = -1;
            cell.terminalGoalIndex = terminalIndex;
            cell.reachable = true;
            openSet.push({ cellIndex, initialCost });
        }

        constexpr std::array<Tile, 8> DIRECTIONS = {
            Tile{ 1, 0 }, Tile{ -1, 0 }, Tile{ 0, 1 }, Tile{ 0, -1 },
            Tile{ 1, 1 }, Tile{ 1, -1 }, Tile{ -1, 1 }, Tile{ -1, -1 }
        };

        while (!openSet.empty()) {
            FlowQueueNode current = openSet.top();
            openSet.pop();
            const FlowFieldCell& currentCell = field.cells[current.cellIndex];
            if (!currentCell.reachable ||
                current.cost > currentCell.integrationCost) {
                continue;
            }

            ++field.expandedCells;
            Tile currentTile = cache.GetTile(current.cellIndex);
            for (Tile direction : DIRECTIONS) {
                Tile neighbor = {
                    currentTile.x + direction.x,
                    currentTile.y + direction.y
                };
                int neighborIndex = cache.GetIndex(neighbor);
                if (neighborIndex < 0 ||
                    !cache.IsEdgeClear(neighbor, currentTile)) {
                    continue;
                }

                bool diagonal = direction.x != 0 && direction.y != 0;
                float nextCost = current.cost +
                    (diagonal ? DIAGONAL_COST : 1.0f);
                FlowFieldCell& neighborCell = field.cells[neighborIndex];
                if (neighborCell.reachable &&
                    nextCost >= neighborCell.integrationCost) {
                    continue;
                }

                neighborCell.integrationCost = nextCost;
                neighborCell.nextCellIndex = current.cellIndex;
                neighborCell.terminalGoalIndex =
                    currentCell.terminalGoalIndex;
                neighborCell.reachable = true;
                openSet.push({ neighborIndex, nextCost });
            }
        }

        field.available = !field.terminals.empty();
        return field;
    }

    /// Searches for best flow start connection.
    std::optional<PositionConnection> FindBestFlowStartConnection(
        LevelManager& levelManager,
        Enemy& enemy,
        SearchCollisionCache& cache,
        const FlowFieldData& field,
        Rectangle searchBounds
    ) {
        Vector2 start = NavigationCenterAtEntityPosition(
            enemy,
            enemy.GetPosition()
        );
        Tile startTile = WorldTile(levelManager, start);
        std::optional<PositionConnection> best;
        float bestTotalCost = std::numeric_limits<float>::max();

        auto consider = [&](Tile graphTile, std::vector<Vector2> points) {
            int cellIndex = cache.GetIndex(graphTile);
            if (cellIndex < 0 ||
                cellIndex >= (int)field.cells.size() ||
                !field.cells[cellIndex].reachable) {
                return;
            }
            float distance = ConnectionLength(start, points);
            float totalCost = distance / Constants::RENDER_TILE_SIZE +
                field.cells[cellIndex].integrationCost;
            if (totalCost < bestTotalCost) {
                bestTotalCost = totalCost;
                best = PositionConnection{
                    graphTile,
                    std::move(points),
                    distance
                };
            }
        };

        std::vector<Tile> graphTiles = NearbyTiles(startTile);
        for (Tile graphTile : graphTiles) {
            if (!cache.IsTileClear(graphTile)) continue;
            Vector2 center = levelManager.TileToWorld(graphTile.x, graphTile.y);
            auto connection = ConnectPositions(
                levelManager,
                enemy,
                start,
                center,
                searchBounds
            );
            if (connection) consider(graphTile, std::move(*connection));
        }
        if (best) return best;

        constexpr std::array<Vector2, 8> ANCHOR_OFFSETS = {
            Vector2{ QUARTER_TILE_OFFSET, 0.0f },
            Vector2{ -QUARTER_TILE_OFFSET, 0.0f },
            Vector2{ 0.0f, QUARTER_TILE_OFFSET },
            Vector2{ 0.0f, -QUARTER_TILE_OFFSET },
            Vector2{ QUARTER_TILE_OFFSET, QUARTER_TILE_OFFSET },
            Vector2{ QUARTER_TILE_OFFSET, -QUARTER_TILE_OFFSET },
            Vector2{ -QUARTER_TILE_OFFSET, QUARTER_TILE_OFFSET },
            Vector2{ -QUARTER_TILE_OFFSET, -QUARTER_TILE_OFFSET }
        };

        int attempts = 0;
        for (Tile graphTile : graphTiles) {
            if (!cache.IsTileClear(graphTile)) continue;
            Vector2 graphCenter = levelManager.TileToWorld(
                graphTile.x,
                graphTile.y
            );
            for (Vector2 offset : ANCHOR_OFFSETS) {
                if (attempts++ >= MAX_START_CONNECTION_ATTEMPTS) return best;
                Vector2 anchor = Vector2Add(graphCenter, offset);
                if (!IsBodyClearAtNavigationCenter(
                        levelManager,
                        enemy,
                        anchor,
                        searchBounds)) {
                    continue;
                }
                auto toAnchor = ConnectPositions(
                    levelManager,
                    enemy,
                    start,
                    anchor,
                    searchBounds
                );
                auto toGraph = ConnectPositions(
                    levelManager,
                    enemy,
                    anchor,
                    graphCenter,
                    searchBounds
                );
                if (!toAnchor || !toGraph) continue;

                std::vector<Vector2> points = *toAnchor;
                points.insert(points.end(), toGraph->begin(), toGraph->end());
                consider(graphTile, std::move(points));
            }
        }
        return best;
    }

    /// Implements the condense waypoints behavior for this component.
    std::vector<Vector2> CondenseWaypoints(
        const LevelManager& levelManager,
        const Enemy& enemy,
        const std::vector<Vector2>& rawWaypoints,
        Rectangle searchBounds
    ) {
        std::vector<Vector2> condensed;
        Vector2 cursor = NavigationCenterAtEntityPosition(
            enemy,
            enemy.GetPosition()
        );
        std::size_t index = 0;

        while (index < rawWaypoints.size() &&
               (int)condensed.size() < MAX_TARGET_POSITIONS) {
            std::size_t farthest = index;
            for (std::size_t candidate = rawWaypoints.size();
                 candidate-- > index;) {
                if (IsNavigationPathClear(
                        levelManager,
                        enemy,
                        cursor,
                        rawWaypoints[candidate],
                        searchBounds)) {
                    farthest = candidate;
                    break;
                }
            }

            Vector2 waypoint = rawWaypoints[farthest];
            if (condensed.empty() ||
                !AlmostSamePosition(condensed.back(), waypoint)) {
                condensed.push_back(waypoint);
            }
            cursor = waypoint;
            index = farthest + 1;
        }

        if (!rawWaypoints.empty() &&
            (condensed.empty() ||
             !AlmostSamePosition(condensed.back(), rawWaypoints.back()))) {
            if ((int)condensed.size() == MAX_TARGET_POSITIONS) {
                condensed.back() = rawWaypoints.back();
            } else {
                condensed.push_back(rawWaypoints.back());
            }
        }

        return condensed;
    }

    /// Implements the convert navigation waypoints to entity positions behavior for this component.
    std::vector<Vector2> ConvertNavigationWaypointsToEntityPositions(
        const Enemy& enemy,
        const std::vector<Vector2>& navigationWaypoints
    ) {
        std::vector<Vector2> entityWaypoints;
        entityWaypoints.reserve(navigationWaypoints.size());
        for (Vector2 navigationCenter : navigationWaypoints) {
            entityWaypoints.push_back(EntityPositionAtNavigationCenter(
                enemy,
                navigationCenter
            ));
        }
        return entityWaypoints;
    }

    /// Searches for path from flow field.
    PathSearchResult FindPathFromFlowField(
        LevelManager& levelManager,
        Enemy& enemy,
        const std::vector<EnemyPathDebugPoint>& candidates,
        const FlowFieldData& field,
        EnemyNavigationCacheStore& store,
        Rectangle searchBounds,
        std::uint64_t navigationRevision
    ) {
        PathSearchResult result;
        result.debugGoals = candidates;
        result.expandedCells = field.expandedCells;
        if (!field.available ||
            !IsBodyClearAtWorldPosition(
                levelManager,
                enemy,
                enemy.GetPosition(),
                searchBounds)) {
            return result;
        }

        Vector2 currentNavigationCenter = NavigationCenterAtEntityPosition(
            enemy,
            enemy.GetPosition()
        );

        for (const EnemyPathDebugPoint& candidate : candidates) {
            if (candidate.hasLineOfSight &&
                DistanceSquared(
                    currentNavigationCenter,
                    candidate.position
                ) <=
                    POSITION_EPSILON_SQUARED) {
                result.status = EnemyPathStatus::AtGoal;
                result.selectedGoal = candidate.position;
                return result;
            }
        }

        SearchCollisionCache cache(
            levelManager,
            enemy,
            store,
            searchBounds,
            navigationRevision
        );
        if (!(cache.GetKey() == field.key)) return result;

        std::optional<PositionConnection> start =
            FindBestFlowStartConnection(
                levelManager,
                enemy,
                cache,
                field,
                searchBounds
            );
        if (!start) return result;

        int currentIndex = cache.GetIndex(start->graphTile);
        if (currentIndex < 0 ||
            currentIndex >= (int)field.cells.size() ||
            !field.cells[currentIndex].reachable) {
            return result;
        }

        int terminalIndex = field.cells[currentIndex].terminalGoalIndex;
        if (terminalIndex < 0 ||
            terminalIndex >= (int)field.terminals.size()) {
            return result;
        }
        const FlowFieldTerminal& terminal = field.terminals[terminalIndex];
        result.selectedGoal = terminal.goalPosition;

        if (IsNavigationPathClear(
                levelManager,
                enemy,
                currentNavigationCenter,
                terminal.goalPosition,
                searchBounds)) {
            result.waypoints = {
                EntityPositionAtNavigationCenter(
                    enemy,
                    terminal.goalPosition
                )
            };
            result.status = EnemyPathStatus::Ready;
            return result;
        }

        std::vector<Vector2> rawWaypoints = start->waypoints;
        std::vector<std::uint8_t> visited(field.cells.size(), 0);
        for (int guard = 0;
             guard < (int)field.cells.size() && currentIndex >= 0;
             ++guard) {
            if (visited[currentIndex] != 0) return PathSearchResult{};
            visited[currentIndex] = 1;

            const FlowFieldCell& cell = field.cells[currentIndex];
            if (cell.terminalGoalIndex != terminalIndex) {
                return PathSearchResult{};
            }
            if (cell.nextCellIndex < 0) break;
            currentIndex = cell.nextCellIndex;
            Tile nextTile = cache.GetTile(currentIndex);
            rawWaypoints.push_back(levelManager.TileToWorld(
                nextTile.x,
                nextTile.y
            ));
        }

        rawWaypoints.insert(
            rawWaypoints.end(),
            terminal.suffix.begin(),
            terminal.suffix.end()
        );
        result.waypoints = ConvertNavigationWaypointsToEntityPositions(
            enemy,
            CondenseWaypoints(
                levelManager,
                enemy,
                rawWaypoints,
                searchBounds
            )
        );
        result.status = result.waypoints.empty()
            ? EnemyPathStatus::AtGoal
            : EnemyPathStatus::Ready;
        return result;
    }

    /// Searches for path to candidates.
    PathSearchResult FindPathToCandidates(
        LevelManager& levelManager,
        Enemy& enemy,
        const std::vector<EnemyPathDebugPoint>& candidates,
        EnemyNavigationCacheStore& store,
        Rectangle searchBounds,
        std::uint64_t navigationRevision
    ) {
        PathSearchResult result;

        if (!IsBodyClearAtWorldPosition(
                levelManager,
                enemy,
                enemy.GetPosition(),
                searchBounds)) {
            return result;
        }

        Vector2 currentNavigationCenter = NavigationCenterAtEntityPosition(
            enemy,
            enemy.GetPosition()
        );

        SearchCollisionCache cache(
            levelManager,
            enemy,
            store,
            searchBounds,
            navigationRevision
        );
        std::optional<PositionConnection> start = FindStartConnection(
            levelManager,
            enemy,
            cache,
            searchBounds
        );
        std::vector<GoalAnchor> goals = BuildGoalAnchors(
            levelManager,
            enemy,
            candidates,
            cache,
            result.debugGoals,
            searchBounds
        );

        for (const EnemyPathDebugPoint& candidate : result.debugGoals) {
            if (candidate.hasLineOfSight &&
                DistanceSquared(
                    currentNavigationCenter,
                    candidate.position
                ) <=
                    POSITION_EPSILON_SQUARED) {
                result.status = EnemyPathStatus::AtGoal;
                result.selectedGoal = candidate.position;
                return result;
            }
        }

        if (!start || goals.empty()) {
            return result;
        }

        std::priority_queue<
            AStarNode,
            std::vector<AStarNode>,
            CompareAStarNode
        > openSet;
        std::unordered_map<Tile, Tile, TileHash> cameFrom;
        std::unordered_map<Tile, float, TileHash> gScore;
        cameFrom.reserve(MAX_SEARCH_STEPS);
        gScore.reserve(MAX_SEARCH_STEPS);

        openSet.push({
            start->graphTile,
            0.0f,
            HeuristicToGoals(start->graphTile, goals)
        });
        gScore[start->graphTile] = 0.0f;

        constexpr std::array<Tile, 8> DIRECTIONS = {
            Tile{ 1, 0 },
            Tile{ -1, 0 },
            Tile{ 0, 1 },
            Tile{ 0, -1 },
            Tile{ 1, 1 },
            Tile{ 1, -1 },
            Tile{ -1, 1 },
            Tile{ -1, -1 }
        };

        int expandedCells = 0;
        bool reachedSearchLimit = false;

        while (!openSet.empty()) {
            AStarNode current = openSet.top();
            openSet.pop();

            auto currentScore = gScore.find(current.tile);
            if (currentScore == gScore.end() ||
                current.gCost > currentScore->second) {
                continue;
            }

            if (expandedCells >= MAX_SEARCH_STEPS) {
                reachedSearchLimit = true;
                break;
            }
            ++expandedCells;
            result.expandedCells = expandedCells;

            const GoalAnchor* reachedGoal = FindGoalAtTile(
                current.tile,
                goals
            );
            if (reachedGoal) {
                std::vector<Tile> tilePath;
                Tile step = current.tile;
                tilePath.push_back(step);
                while (!(step == start->graphTile)) {
                    auto previous = cameFrom.find(step);
                    if (previous == cameFrom.end()) {
                        return result;
                    }
                    step = previous->second;
                    tilePath.push_back(step);
                }
                std::reverse(tilePath.begin(), tilePath.end());

                std::vector<Vector2> rawWaypoints = start->waypoints;
                for (std::size_t index = 1; index < tilePath.size(); ++index) {
                    rawWaypoints.push_back(levelManager.TileToWorld(
                        tilePath[index].x,
                        tilePath[index].y
                    ));
                }
                rawWaypoints.insert(
                    rawWaypoints.end(),
                    reachedGoal->suffix.begin(),
                    reachedGoal->suffix.end()
                );

                result.waypoints = ConvertNavigationWaypointsToEntityPositions(
                    enemy,
                    CondenseWaypoints(
                        levelManager,
                        enemy,
                        rawWaypoints,
                        searchBounds
                    )
                );
                result.status = result.waypoints.empty()
                    ? EnemyPathStatus::AtGoal
                    : EnemyPathStatus::Ready;
                result.selectedGoal = reachedGoal->goalPosition;
                return result;
            }

            for (Tile direction : DIRECTIONS) {
                Tile neighbor = {
                    current.tile.x + direction.x,
                    current.tile.y + direction.y
                };
                if (!cache.IsEdgeClear(current.tile, neighbor)) continue;

                bool diagonal = direction.x != 0 && direction.y != 0;
                float tentativeCost = current.gCost +
                    (diagonal ? DIAGONAL_COST : 1.0f);
                auto existing = gScore.find(neighbor);
                bool isNew = existing == gScore.end();
                if (!isNew && tentativeCost >= existing->second) continue;

                if (isNew && (int)gScore.size() >= MAX_SEARCH_STEPS) {
                    reachedSearchLimit = true;
                    continue;
                }

                cameFrom[neighbor] = current.tile;
                gScore[neighbor] = tentativeCost;
                openSet.push({
                    neighbor,
                    tentativeCost,
                    tentativeCost + HeuristicToGoals(neighbor, goals)
                });
            }
        }

        if (reachedSearchLimit) {
            result.status = EnemyPathStatus::SearchLimitReached;
        }
        result.expandedCells = expandedCells;
        return result;
    }

    /// Searches for path to player.
    PathSearchResult FindPathToPlayer(
        LevelManager& levelManager,
        Enemy& enemy,
        Paladin& target,
        EnemyNavigationCacheStore& store,
        Rectangle searchBounds,
        std::uint64_t navigationRevision
    ) {
        const std::vector<EnemyPathDebugPoint>& candidates =
            GetSharedGoalCandidates(
                levelManager,
                target,
                store,
                searchBounds
            );
        return FindPathToCandidates(
            levelManager,
            enemy,
            candidates,
            store,
            searchBounds,
            navigationRevision
        );
    }

    /// Searches for path to explicit goal.
    PathSearchResult FindPathToExplicitGoal(
        LevelManager& levelManager,
        Enemy& enemy,
        Vector2 worldGoal,
        EnemyNavigationCacheStore& store,
        Rectangle searchBounds,
        std::uint64_t navigationRevision
    ) {
        const std::vector<EnemyPathDebugPoint> candidates = {
            {
                NavigationCenterAtEntityPosition(enemy, worldGoal),
                true
            }
        };
        return FindPathToCandidates(
            levelManager,
            enemy,
            candidates,
            store,
            searchBounds,
            navigationRevision
        );
    }

    /// Reports whether this component has reached waypoint.
    bool HasReachedWaypoint(const Enemy& enemy, Vector2 waypoint) {
        Rectangle body = enemy.GetCollisionBox();
        float reachDistance = std::max(
            4.0f,
            std::min(body.width, body.height) * 0.25f
        );
        return DistanceSquared(enemy.GetPosition(), waypoint) <=
            reachDistance * reachDistance;
    }

    /// Implements the pop reached targets behavior for this component.
    void PopReachedTargets(Enemy& enemy) {
        while (enemy.HasTargetPosition() &&
               HasReachedWaypoint(enemy, enemy.FirstTargetPosition())) {
            enemy.PopTarget();
        }
    }
}

/// Creates a PathFindingManager instance from the supplied configuration.
PathFindingManager::PathFindingManager(
    LevelManager& levelManager,
    ObjectManager& objectManager
)
    : levelManager(levelManager),
      objectManager(objectManager),
      navigationCacheStore(
          std::make_unique<EnemyNavigationCacheStore>()) {
}

/// Releases resources owned by this PathFindingManager instance.
PathFindingManager::~PathFindingManager() = default;

/// Begins path finding.
void PathFindingManager::BeginPathFinding(Enemy& enemy) {
    AddEnemy(enemy);
}

/// Begins path finding to.
void PathFindingManager::BeginPathFindingTo(
    Enemy& enemy,
    Vector2 worldGoal
) {
    AddEnemyTo(enemy, worldGoal);
}

/// Finishes path finding.
void PathFindingManager::EndPathFinding(Enemy& enemy) {
    RemoveEnemy(enemy);
}

/// Reports whether the blocked condition is satisfied.
bool PathFindingManager::IsBlocked(Rectangle bounds) const {
    return levelManager.IsSolidCollision(bounds);
}

/// Returns the current level bounds.
Rectangle PathFindingManager::GetLevelBounds() const {
    // During combat, collision recovery must stay on the room side of a
    // locked gate instead of selecting a clear point in the corridor.
    return levelManager.GetCurrentRoomBounds();
}

/// Returns the current navigable tile centers within.
std::vector<Vector2> PathFindingManager::GetNavigableTileCentersWithin(
    const Enemy& enemy,
    Vector2 origin,
    float radius
) const {
    std::vector<Vector2> candidates;
    if (radius <= 0.0f) return candidates;

    Rectangle searchBounds = levelManager.GetCurrentRoomBounds();
    Vector2 originNavigationCenter = NavigationCenterAtEntityPosition(
        enemy,
        origin
    );
    float tileSize = Constants::RENDER_TILE_SIZE;
    int minimumTileX = (int)std::floor(
        std::max(
            searchBounds.x,
            originNavigationCenter.x - radius
        ) / tileSize
    );
    int maximumTileX = (int)std::floor(
        std::min(searchBounds.x + searchBounds.width,
                 originNavigationCenter.x + radius) / tileSize
    );
    int minimumTileY = (int)std::floor(
        std::max(
            searchBounds.y,
            originNavigationCenter.y - radius
        ) / tileSize
    );
    int maximumTileY = (int)std::floor(
        std::min(searchBounds.y + searchBounds.height,
                 originNavigationCenter.y + radius) / tileSize
    );
    Vector2 originTile = levelManager.WorldToTile(originNavigationCenter);
    float radiusSquared = radius * radius;

    auto isInsideSearchBounds = [searchBounds](Rectangle bounds) {
        constexpr float BOUNDS_TOLERANCE = 0.001f;
        return bounds.x >= searchBounds.x - BOUNDS_TOLERANCE &&
            bounds.y >= searchBounds.y - BOUNDS_TOLERANCE &&
            bounds.x + bounds.width <=
                searchBounds.x + searchBounds.width + BOUNDS_TOLERANCE &&
            bounds.y + bounds.height <=
                searchBounds.y + searchBounds.height + BOUNDS_TOLERANCE;
    };

    for (int tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
        for (int tileX = minimumTileX; tileX <= maximumTileX; ++tileX) {
            if (tileX == (int)originTile.x &&
                tileY == (int)originTile.y) {
                continue;
            }

            Vector2 navigationCenter = levelManager.TileToWorld(tileX, tileY);
            Vector2 delta = Vector2Subtract(
                navigationCenter,
                originNavigationCenter
            );
            if (Vector2LengthSqr(delta) > radiusSquared) continue;

            Rectangle footprint = NavigationFootprintAtCenter(
                enemy,
                navigationCenter
            );
            if (!isInsideSearchBounds(footprint) ||
                levelManager.IsSolidCollision(footprint)) {
                continue;
            }
            candidates.push_back(EntityPositionAtNavigationCenter(
                enemy,
                navigationCenter
            ));
        }
    }
    return candidates;
}

/// Removes enemy.
void PathFindingManager::RemoveEnemy(Enemy& enemy) {
    ObjectId enemyId = enemy.GetObjectId();
    enemies.erase(
        std::remove(enemies.begin(), enemies.end(), enemyId),
        enemies.end()
    );
    pathRecords.erase(enemyId);

    if (nextEnemyIndex >= (int)enemies.size()) {
        nextEnemyIndex = 0;
    }
    searchCredits = std::min(searchCredits, (float)enemies.size());
}

/// Adds enemy.
void PathFindingManager::AddEnemy(Enemy& enemy) {
    ObjectId enemyId = enemy.GetObjectId();
    if (std::find(enemies.begin(), enemies.end(), enemyId) == enemies.end()) {
        enemies.push_back(enemyId);
        pathRecords[enemyId] = PathRecord{};
        enemy.SetPathStatus(EnemyPathStatus::Pending);
    }
}

/// Removes all runtime entries owned by this component and resets transient state.
void PathFindingManager::Clear() {
    decltype(enemies){}.swap(enemies);
    decltype(pathRecords){}.swap(pathRecords);
    nextEnemyIndex = 0;
    searchCredits = 0.0f;
    decltype(navigationCacheStore->grids){}.swap(
        navigationCacheStore->grids
    );
    navigationCacheStore->goalsValid = false;
    navigationCacheStore->goalTarget = nullptr;
    navigationCacheStore->goalTargetBounds = {};
    navigationCacheStore->goalRevision = 0;
    decltype(navigationCacheStore->sharedGoals){}.swap(
        navigationCacheStore->sharedGoals
    );
    decltype(navigationCacheStore->flowFields){}.swap(
        navigationCacheStore->flowFields
    );
}

/// Returns the current memory stats.
PathFindingMemoryStats PathFindingManager::GetMemoryStats() const {
    PathFindingMemoryStats stats;
    stats.enemies = enemies.size();
    stats.enemyCapacity = enemies.capacity();
    stats.pathRecords = pathRecords.size();
    stats.navigationGrids = navigationCacheStore->grids.size();
    for (const auto& entry : navigationCacheStore->grids) {
        stats.navigationGridCells += entry.second.clearTiles.capacity();
        stats.navigationGridCells += entry.second.clearEdges.capacity();
    }
    stats.flowFields = navigationCacheStore->flowFields.size();
    for (const auto& entry : navigationCacheStore->flowFields) {
        stats.flowFieldCells += entry.second.cells.capacity();
        for (const FlowFieldTerminal& terminal : entry.second.terminals) {
            stats.flowFieldCells += terminal.suffix.capacity();
        }
    }
    stats.sharedGoals = navigationCacheStore->sharedGoals.size();
    stats.sharedGoalCapacity = navigationCacheStore->sharedGoals.capacity();
    return stats;
}

/// Adds enemy to.
void PathFindingManager::AddEnemyTo(Enemy& enemy, Vector2 worldGoal) {
    AddEnemy(enemy);
    PathRecord& record = pathRecords[enemy.GetObjectId()];
    record.mode = NavigationMode::ExplicitGoalAStar;
    record.explicitGoal = worldGoal;
    record.hasTargetTile = false;
    record.forceRepath = true;
    record.lastSearchFailed = false;
    record.pathAge = 0.0f;
    searchCredits = std::min(
        (float)enemies.size(),
        searchCredits + 1.0f
    );
    enemy.SetPathStatus(EnemyPathStatus::Pending);
}

/// Removes reached waypoints and returns the next safe entity-space target.
/// A waypoint invalidated by changed map collision clears the route and requests
/// a repath instead of allowing the enemy to move through the new blocker.
std::optional<Vector2> PathFindingManager::GetNextMoveTarget(
    Enemy& enemy
) {
    PopReachedTargets(enemy);

    if (!enemy.HasTargetPosition()) {
        if (enemy.GetPathStatus() == EnemyPathStatus::Ready) {
            enemy.SetPathStatus(EnemyPathStatus::AtGoal);
        }
        return std::nullopt;
    }

    Vector2 targetPosition = enemy.FirstTargetPosition();
    Rectangle searchBounds = levelManager.GetCurrentRoomBounds();
    if (!IsBodyClearAtWorldPosition(
            levelManager,
            enemy,
            targetPosition,
            searchBounds)) {
        enemy.ClearTargetPosition();
        enemy.ClearSelectedPathGoal();
        enemy.SetPathStatus(EnemyPathStatus::Pending);
        auto record = pathRecords.find(enemy.GetObjectId());
        if (record != pathRecords.end()) {
            record->second.forceRepath = true;
        }
        searchCredits = std::min(
            (float)std::max<std::size_t>(1, enemies.size()),
            searchCredits + 1.0f
        );
        return std::nullopt;
    }

    return targetPosition;
}

/// Applies short-range steering to a direction supplied by global pathfinding.
/// Walls always win; enemy separation is blended only when side clearance exists,
/// allowing narrow corridors to ignore crowds without entering map geometry.
Vector2 PathFindingManager::GetLocalDirection(
    Enemy& enemy,
    Vector2 desiredDirection
) {
    if (Vector2Length(desiredDirection) <= 0.001f) {
        return { 0.0f, 0.0f };
    }
    desiredDirection = Vector2Normalize(desiredDirection);
    if (!enemy.IsLocalEnemyAvoidanceEnabled()) {
        return desiredDirection;
    }

    Vector2 enemyPosition = enemy.GetPosition();
    Rectangle searchBounds = levelManager.GetCurrentRoomBounds();
    constexpr float FORWARD_PROBE_DISTANCE = 18.0f;
    constexpr float SIDE_CLEARANCE_DISTANCE = 12.0f;
    constexpr float SEPARATION_RADIUS = 36.0f;
    constexpr float SEPARATION_RADIUS_SQUARED =
        SEPARATION_RADIUS * SEPARATION_RADIUS;
    constexpr float SEPARATION_WEIGHT = 0.4f;
    constexpr float MINIMUM_FORWARD_DOT = 0.5f;

    auto isDirectionClear = [&](Vector2 direction, float distance) {
        return IsBodyPathClear(
            levelManager,
            enemy,
            enemyPosition,
            Vector2Add(
                enemyPosition,
                Vector2Scale(direction, distance)
            ),
            searchBounds
        );
    };

    Vector2 left = { -desiredDirection.y, desiredDirection.x };
    Vector2 right = { desiredDirection.y, -desiredDirection.x };
    bool desiredClear = isDirectionClear(
        desiredDirection,
        FORWARD_PROBE_DISTANCE
    );
    if (!desiredClear) {
        Vector2 leftForward = Vector2Normalize(Vector2Add(
            desiredDirection,
            Vector2Scale(left, 0.75f)
        ));
        Vector2 rightForward = Vector2Normalize(Vector2Add(
            desiredDirection,
            Vector2Scale(right, 0.75f)
        ));
        bool leftForwardClear = isDirectionClear(
            leftForward,
            FORWARD_PROBE_DISTANCE
        );
        bool rightForwardClear = isDirectionClear(
            rightForward,
            FORWARD_PROBE_DISTANCE
        );
        if (leftForwardClear && !rightForwardClear) return leftForward;
        if (rightForwardClear && !leftForwardClear) return rightForward;
        if (leftForwardClear && rightForwardClear) return leftForward;
        return desiredDirection;
    }

    // Crowd avoidance is optional. In a narrow passage, ignore nearby enemies
    // and keep following the globally valid map path.
    if (!isDirectionClear(left, SIDE_CLEARANCE_DISTANCE) ||
        !isDirectionClear(right, SIDE_CLEARANCE_DISTANCE)) {
        return desiredDirection;
    }

    Vector2 separation = { 0.0f, 0.0f };

    objectManager.GetEnemiesNear(
        {
            enemyPosition.x - SEPARATION_RADIUS,
            enemyPosition.y - SEPARATION_RADIUS,
            SEPARATION_RADIUS * 2.0f,
            SEPARATION_RADIUS * 2.0f
        },
        nearbyEnemyScratch
    );
    for (Enemy* otherEnemy : nearbyEnemyScratch) {
        if (!otherEnemy) continue;
        if (otherEnemy == &enemy ||
            !otherEnemy->IsLocalEnemyAvoidanceEnabled() ||
            otherEnemy->IsDead() ||
            !otherEnemy->IsEnabled()) continue;
        if (!ContainsRectangle(
                searchBounds,
                otherEnemy->GetCollisionBox())) {
            continue;
        }

        Vector2 away = Vector2Subtract(
            enemyPosition,
            otherEnemy->GetPosition()
        );
        float distanceSquared = away.x * away.x + away.y * away.y;
        if (distanceSquared > 0.000001f &&
            distanceSquared < SEPARATION_RADIUS_SQUARED) {
            float distance = std::sqrt(distanceSquared);
            float strength = (SEPARATION_RADIUS - distance) /
                SEPARATION_RADIUS;
            separation = Vector2Add(
                separation,
                Vector2Scale(
                    Vector2Normalize(away),
                    strength * SEPARATION_WEIGHT
                )
            );
        }
    }

    if (Vector2LengthSqr(separation) <= 0.000001f) {
        return desiredDirection;
    }

    Vector2 candidateDirection = Vector2Normalize(Vector2Add(
        desiredDirection,
        separation
    ));
    if (Vector2DotProduct(candidateDirection, desiredDirection) <
            MINIMUM_FORWARD_DOT ||
        !isDirectionClear(candidateDirection, FORWARD_PROBE_DISTANCE)) {
        return desiredDirection;
    }
    return candidateDirection;
}

/// Services registered enemies under a per-frame search budget.
/// Shared player goals use cached navigation/flow-field data, while explicit
/// destinations (such as Drone patrol tiles) retain bounded A* path records.
void PathFindingManager::Update(float deltaTime) {
    profilingStats.flowFieldBuildsThisFrame = 0;
    profilingStats.searchesThisFrame = 0;
    profilingTimer += std::max(0.0f, deltaTime);
    if (profilingTimer >= 1.0f) {
        profilingStats.flowFieldBuildsLastSecond =
            profilingFlowFieldBuilds;
        profilingStats.flowFieldCacheHitsLastSecond =
            profilingFlowFieldCacheHits;
        profilingStats.averageFlowFieldExpandedCells =
            profilingFlowFieldBuilds > 0
                ? (float)profilingFlowExpandedTotal /
                    (float)profilingFlowFieldBuilds
                : 0.0f;
        profilingStats.maximumFlowFieldExpandedCells =
            profilingFlowExpandedMaximum;
        profilingStats.averageFlowFieldMilliseconds =
            profilingFlowFieldBuilds > 0
                ? profilingFlowMillisecondsTotal /
                    (float)profilingFlowFieldBuilds
                : 0.0f;
        profilingStats.maximumFlowFieldMilliseconds =
            profilingFlowMillisecondsMaximum;
        profilingStats.searchesLastSecond = profilingSearches;
        profilingStats.readyLastSecond = profilingReady;
        profilingStats.unreachableLastSecond = profilingUnreachable;
        profilingStats.searchLimitLastSecond = profilingSearchLimit;
        profilingStats.averageExpandedCells = profilingSearches > 0
            ? (float)profilingExpandedTotal / (float)profilingSearches
            : 0.0f;
        profilingStats.maximumExpandedCells = profilingExpandedMaximum;
        profilingStats.averageSearchMilliseconds = profilingSearches > 0
            ? profilingMillisecondsTotal / (float)profilingSearches
            : 0.0f;
        profilingStats.maximumSearchMilliseconds =
            profilingMillisecondsMaximum;

        profilingTimer = std::fmod(profilingTimer, 1.0f);
        profilingFlowFieldBuilds = 0;
        profilingFlowFieldCacheHits = 0;
        profilingFlowExpandedTotal = 0;
        profilingFlowExpandedMaximum = 0;
        profilingFlowMillisecondsTotal = 0.0f;
        profilingFlowMillisecondsMaximum = 0.0f;
        profilingSearches = 0;
        profilingReady = 0;
        profilingUnreachable = 0;
        profilingSearchLimit = 0;
        profilingExpandedTotal = 0;
        profilingExpandedMaximum = 0;
        profilingMillisecondsTotal = 0.0f;
        profilingMillisecondsMaximum = 0.0f;
    }

    if (enemies.empty() || TARGET_LOOP_ALL_INTERVAL <= 0.0f) {
        searchCredits = 0.0f;
        profilingStats.activeFlowFieldProfiles = 0;
        return;
    }

    for (auto& entry : pathRecords) {
        entry.second.pathAge += std::max(0.0f, deltaTime);
    }

    std::uint64_t navigationRevision =
        levelManager.GetNavigationRevision();
    Rectangle searchBounds = levelManager.GetCurrentRoomBounds();
    navigationCacheStore->EnsureRevision(navigationRevision);

    auto getPlayerTarget = [](Enemy& enemy) -> Paladin* {
        TeamManager* targetTeam = enemy.GetTargetTeam();
        return targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    };

    auto applyResult = [&](Enemy& enemy,
                           PathRecord& record,
                           PathSearchResult result,
                           Tile targetTile,
                           bool navigationChanged,
                           bool goalChanged) {
        record.hasTargetTile = true;
        record.targetTileX = targetTile.x;
        record.targetTileY = targetTile.y;
        record.navigationRevision = navigationRevision;
        record.goalRevision = navigationCacheStore->goalRevision;
        record.pathAge = 0.0f;
        record.forceRepath = false;
        record.lastSearchFailed =
            result.status == EnemyPathStatus::Unreachable ||
            result.status == EnemyPathStatus::SearchLimitReached;

        enemy.SetPathDebugPoints(std::move(result.debugGoals));
        if (result.status == EnemyPathStatus::Ready) {
            enemy.ClearTargetPosition();
            for (Vector2 waypoint : result.waypoints) {
                enemy.AddTargetPosition(waypoint);
            }
            enemy.SetPathStatus(EnemyPathStatus::Ready);
            enemy.ClearSelectedPathGoal();
            if (result.selectedGoal) {
                enemy.SetSelectedPathGoal(*result.selectedGoal);
            }
            return;
        }

        if (result.status == EnemyPathStatus::AtGoal) {
            enemy.ClearTargetPosition();
            enemy.SetPathStatus(EnemyPathStatus::AtGoal);
            enemy.ClearSelectedPathGoal();
            if (result.selectedGoal) {
                enemy.SetSelectedPathGoal(*result.selectedGoal);
            }
            return;
        }

        bool existingPathValid = !navigationChanged && !goalChanged &&
            enemy.HasTargetPosition() &&
            IsBodyPathClear(
                levelManager,
                enemy,
                enemy.GetPosition(),
                enemy.FirstTargetPosition(),
                searchBounds
            );
        if (existingPathValid) {
            enemy.SetPathStatus(EnemyPathStatus::Ready);
            return;
        }

        enemy.ClearTargetPosition();
        enemy.ClearSelectedPathGoal();
        enemy.SetPathStatus(result.status);
    };

    bool builtFlowField = false;
    std::unordered_set<
        NavigationCacheKey,
        NavigationCacheKeyHash
    > activeFlowProfiles;
    std::unordered_set<
        NavigationCacheKey,
        NavigationCacheKeyHash
    > observedCachedProfiles;

    for (ObjectId enemyId : enemies) {
        auto recordIt = pathRecords.find(enemyId);
        Enemy* enemy = objectManager.FindEnemy(enemyId);
        if (recordIt == pathRecords.end() || !enemy || enemy->IsDead() ||
            !enemy->IsEnabled() ||
            recordIt->second.mode != NavigationMode::PlayerFlowField) {
            continue;
        }

        Paladin* target = getPlayerTarget(*enemy);
        if (!target) continue;
        const std::vector<EnemyPathDebugPoint>& candidates =
            GetSharedGoalCandidates(
                levelManager,
                *target,
                *navigationCacheStore,
                searchBounds
            );
        SearchCollisionCache cache(
            levelManager,
            *enemy,
            *navigationCacheStore,
            searchBounds,
            navigationRevision
        );
        NavigationCacheKey key = cache.GetKey();
        activeFlowProfiles.insert(key);

        auto fieldIt = navigationCacheStore->flowFields.find(key);
        if (fieldIt != navigationCacheStore->flowFields.end()) {
            if (observedCachedProfiles.insert(key).second) {
                ++profilingFlowFieldCacheHits;
            }
            continue;
        }
        if (builtFlowField) continue;

        std::chrono::steady_clock::time_point buildStarted;
        if (Constants::DEBUG_SHOW_PATHFINDING_PROFILING) {
            buildStarted = std::chrono::steady_clock::now();
        }
        FlowFieldData field = BuildFlowField(
            levelManager,
            *enemy,
            candidates,
            *navigationCacheStore,
            searchBounds,
            navigationRevision
        );
        float elapsedMilliseconds = 0.0f;
        if (Constants::DEBUG_SHOW_PATHFINDING_PROFILING) {
            elapsedMilliseconds = std::chrono::duration<float, std::milli>(
                std::chrono::steady_clock::now() - buildStarted
            ).count();
        }

        int expandedCells = field.expandedCells;
        navigationCacheStore->flowFields.emplace(key, std::move(field));
        builtFlowField = true;
        ++profilingStats.flowFieldBuildsThisFrame;
        ++profilingFlowFieldBuilds;
        profilingFlowExpandedTotal += expandedCells;
        profilingFlowExpandedMaximum = std::max(
            profilingFlowExpandedMaximum,
            expandedCells
        );
        profilingFlowMillisecondsTotal += elapsedMilliseconds;
        profilingFlowMillisecondsMaximum = std::max(
            profilingFlowMillisecondsMaximum,
            elapsedMilliseconds
        );
    }
    for (auto field = navigationCacheStore->flowFields.begin();
         field != navigationCacheStore->flowFields.end();) {
        if (activeFlowProfiles.count(field->first) == 0) {
            field = navigationCacheStore->flowFields.erase(field);
        } else {
            ++field;
        }
    }
    profilingStats.activeFlowFieldProfiles =
        (int)activeFlowProfiles.size();

    for (ObjectId enemyId : enemies) {
        auto recordIt = pathRecords.find(enemyId);
        Enemy* enemy = objectManager.FindEnemy(enemyId);
        if (recordIt == pathRecords.end() || !enemy || enemy->IsDead() ||
            !enemy->IsEnabled()) {
            continue;
        }
        PathRecord& record = recordIt->second;
        if (record.mode != NavigationMode::PlayerFlowField) continue;

        Paladin* target = getPlayerTarget(*enemy);
        if (!target) continue;
        Tile targetTile = PlayerCollisionTile(levelManager, *target);
        bool targetChanged = !record.hasTargetTile ||
            record.targetTileX != targetTile.x ||
            record.targetTileY != targetTile.y;
        bool navigationChanged =
            record.navigationRevision != navigationRevision;
        bool goalChanged =
            record.goalRevision != navigationCacheStore->goalRevision;
        bool failedPathRetry = record.lastSearchFailed &&
            record.pathAge >= FAILED_PATH_RETRY_INTERVAL;
        bool needsRoute = record.forceRepath || targetChanged ||
            navigationChanged || goalChanged || failedPathRetry;
        if (!needsRoute) continue;

        const std::vector<EnemyPathDebugPoint>& candidates =
            GetSharedGoalCandidates(
                levelManager,
                *target,
                *navigationCacheStore,
                searchBounds
            );
        SearchCollisionCache cache(
            levelManager,
            *enemy,
            *navigationCacheStore,
            searchBounds,
            navigationRevision
        );
        auto fieldIt = navigationCacheStore->flowFields.find(cache.GetKey());
        if (fieldIt == navigationCacheStore->flowFields.end()) {
            if (navigationChanged || goalChanged) {
                enemy->ClearTargetPosition();
                enemy->ClearSelectedPathGoal();
            }
            enemy->SetPathStatus(EnemyPathStatus::Pending);
            continue;
        }
        if (fieldIt->second.oversized) continue;

        PopReachedTargets(*enemy);
        PathSearchResult result = FindPathFromFlowField(
            levelManager,
            *enemy,
            candidates,
            fieldIt->second,
            *navigationCacheStore,
            searchBounds,
            navigationRevision
        );
        applyResult(
            *enemy,
            record,
            std::move(result),
            targetTile,
            navigationChanged,
            goalChanged
        );
    }

    auto usesAStar = [&](Enemy& enemy, PathRecord& record) {
        if (record.mode == NavigationMode::ExplicitGoalAStar) return true;
        Paladin* target = getPlayerTarget(enemy);
        if (!target) return false;
        SearchCollisionCache cache(
            levelManager,
            enemy,
            *navigationCacheStore,
            searchBounds,
            navigationRevision
        );
        auto fieldIt = navigationCacheStore->flowFields.find(cache.GetKey());
        return fieldIt != navigationCacheStore->flowFields.end() &&
            fieldIt->second.oversized;
    };

    int aStarRequestCount = 0;
    for (ObjectId enemyId : enemies) {
        auto recordIt = pathRecords.find(enemyId);
        Enemy* enemy = objectManager.FindEnemy(enemyId);
        if (recordIt != pathRecords.end() && enemy && !enemy->IsDead() &&
            enemy->IsEnabled() && usesAStar(*enemy, recordIt->second)) {
            ++aStarRequestCount;
        }
    }

    if (aStarRequestCount <= 0) {
        searchCredits = 0.0f;
        return;
    }
    searchCredits += std::max(0.0f, deltaTime) *
        (float)aStarRequestCount / TARGET_LOOP_ALL_INTERVAL;
    searchCredits = std::clamp(
        searchCredits,
        0.0f,
        (float)aStarRequestCount
    );
    int availableSearches = std::min(
        (int)std::floor(searchCredits),
        MAX_SEARCHES_PER_FRAME
    );
    if (availableSearches <= 0) return;

    int searchesPerformed = 0;
    int enemiesInspected = 0;

    while (searchesPerformed < availableSearches &&
           enemiesInspected < (int)enemies.size()) {
        if (nextEnemyIndex >= (int)enemies.size()) nextEnemyIndex = 0;
        ObjectId enemyId = enemies[nextEnemyIndex++];
        Enemy* enemy = objectManager.FindEnemy(enemyId);
        ++enemiesInspected;
        if (!enemy || enemy->IsDead() || !enemy->IsEnabled()) continue;

        auto recordIt = pathRecords.find(enemyId);
        if (recordIt == pathRecords.end()) continue;
        PathRecord& record = recordIt->second;
        if (!usesAStar(*enemy, record)) continue;

        Paladin* target = nullptr;
        Tile targetTile = WorldTile(
            levelManager,
            NavigationCenterAtEntityPosition(
                *enemy,
                record.explicitGoal
            )
        );
        if (record.mode == NavigationMode::PlayerFlowField) {
            target = getPlayerTarget(*enemy);
            if (!target) continue;
            targetTile = PlayerCollisionTile(levelManager, *target);
        }
        bool targetChanged = !record.hasTargetTile ||
            record.targetTileX != targetTile.x ||
            record.targetTileY != targetTile.y;
        bool navigationChanged =
            record.navigationRevision != navigationRevision;
        bool goalChanged = record.mode == NavigationMode::PlayerFlowField &&
            record.goalRevision != navigationCacheStore->goalRevision;
        bool failedPathRetry = record.lastSearchFailed &&
            record.pathAge >= FAILED_PATH_RETRY_INTERVAL;
        bool needsSearch = record.forceRepath || targetChanged ||
            navigationChanged || goalChanged || failedPathRetry;
        if (!needsSearch) continue;

        PopReachedTargets(*enemy);
        std::chrono::steady_clock::time_point searchStarted;
        if (Constants::DEBUG_SHOW_PATHFINDING_PROFILING) {
            searchStarted = std::chrono::steady_clock::now();
        }
        PathSearchResult result =
            record.mode == NavigationMode::ExplicitGoalAStar
            ? FindPathToExplicitGoal(
                levelManager,
                *enemy,
                record.explicitGoal,
                *navigationCacheStore,
                searchBounds,
                navigationRevision
            )
            : FindPathToPlayer(
                levelManager,
                *enemy,
                *target,
                *navigationCacheStore,
                searchBounds,
                navigationRevision
            );
        float elapsedMilliseconds = 0.0f;
        if (Constants::DEBUG_SHOW_PATHFINDING_PROFILING) {
            elapsedMilliseconds = std::chrono::duration<float, std::milli>(
                std::chrono::steady_clock::now() - searchStarted
            ).count();
        }

        ++searchesPerformed;
        searchCredits = std::max(0.0f, searchCredits - 1.0f);
        ++profilingStats.searchesThisFrame;
        ++profilingSearches;
        profilingExpandedTotal += result.expandedCells;
        profilingExpandedMaximum = std::max(
            profilingExpandedMaximum,
            result.expandedCells
        );
        profilingMillisecondsTotal += elapsedMilliseconds;
        profilingMillisecondsMaximum = std::max(
            profilingMillisecondsMaximum,
            elapsedMilliseconds
        );

        if (result.status == EnemyPathStatus::Ready) {
            ++profilingReady;
        } else if (result.status == EnemyPathStatus::SearchLimitReached) {
            ++profilingSearchLimit;
        } else if (result.status == EnemyPathStatus::Unreachable) {
            ++profilingUnreachable;
        }

        applyResult(
            *enemy,
            record,
            std::move(result),
            targetTile,
            navigationChanged,
            goalChanged
        );
    }
}
