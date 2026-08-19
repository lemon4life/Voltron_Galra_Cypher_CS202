#include "Core/Manager/EnemyPathManager.h"

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
    std::vector<EnemyPathDebugPoint> sharedGoals;

    void EnsureRevision(std::uint64_t revision) {
        if (navigationRevision == revision) return;
        navigationRevision = revision;
        grids.clear();
        goalsValid = false;
        sharedGoals.clear();
    }
};

namespace {
    constexpr float DIAGONAL_COST = 1.41421356f;
    constexpr float TARGET_LOOP_ALL_INTERVAL = 0.6f;
    constexpr float BODY_PATH_SAMPLE_SPACING = 4.0f;
    constexpr float POSITION_EPSILON_SQUARED = 4.0f;
    constexpr float PLAYER_GOAL_RADIUS = 30.0f;
    constexpr float QUARTER_TILE_OFFSET = 8.0f;
    constexpr int MAX_SEARCH_STEPS = 70;
    constexpr int MAX_TARGET_POSITIONS = 7;
    constexpr int MAX_SEARCHES_PER_FRAME = 1;
    constexpr int MAX_START_CONNECTION_ATTEMPTS = 32;
    constexpr float FAILED_PATH_RETRY_INTERVAL = 1.2f;

    struct Tile {
        int x;
        int y;

        bool operator==(const Tile& other) const {
            return x == other.x && y == other.y;
        }
    };

    struct TileHash {
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

    float DistanceSquared(Vector2 first, Vector2 second) {
        float dx = first.x - second.x;
        float dy = first.y - second.y;
        return dx * dx + dy * dy;
    }

    bool AlmostSamePosition(Vector2 first, Vector2 second) {
        return DistanceSquared(first, second) < POSITION_EPSILON_SQUARED;
    }

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

    float ConnectionLength(Vector2 start, const std::vector<Vector2>& points) {
        float length = 0.0f;
        Vector2 previous = start;
        for (Vector2 point : points) {
            length += Vector2Distance(previous, point);
            previous = point;
        }
        return length;
    }

    float Heuristic(Tile start, Tile goal) {
        int dx = std::abs(start.x - goal.x);
        int dy = std::abs(start.y - goal.y);
        int straightSteps = std::abs(dx - dy);
        int diagonalSteps = std::min(dx, dy);
        return (float)straightSteps + DIAGONAL_COST * (float)diagonalSteps;
    }

    float HeuristicToGoals(Tile tile, const std::vector<GoalAnchor>& goals) {
        float best = std::numeric_limits<float>::max();
        for (const GoalAnchor& goal : goals) {
            best = std::min(best, Heuristic(tile, goal.graphTile));
        }
        return best;
    }

    bool ContainsRectangle(Rectangle outer, Rectangle inner) {
        constexpr float EDGE_PADDING = 0.001f;
        return inner.x >= outer.x - EDGE_PADDING &&
            inner.y >= outer.y - EDGE_PADDING &&
            inner.x + inner.width <= outer.x + outer.width + EDGE_PADDING &&
            inner.y + inner.height <= outer.y + outer.height + EDGE_PADDING;
    }

    bool IsBodyClearAtWorldPosition(
        const LevelManager& levelManager,
        const Enemy& enemy,
        Vector2 worldPosition,
        Rectangle searchBounds
    ) {
        Rectangle footprint = enemy.GetNavigationFootprintAt(worldPosition);
        return ContainsRectangle(searchBounds, footprint) &&
            !levelManager.IsSolidCollision(footprint);
    }

    bool IsBodyPathClear(
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
            Vector2 position = Vector2Lerp(start, end, amount);
            if (!IsBodyClearAtWorldPosition(
                    levelManager,
                    enemy,
                    position,
                    searchBounds)) {
                return false;
            }
        }

        return true;
    }

    std::optional<std::vector<Vector2>> ConnectPositions(
        const LevelManager& levelManager,
        const Enemy& enemy,
        Vector2 start,
        Vector2 end,
        Rectangle searchBounds
    ) {
        if (IsBodyPathClear(
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
            if (!IsBodyClearAtWorldPosition(
                    levelManager,
                    enemy,
                    corner,
                    searchBounds) ||
                !IsBodyPathClear(
                    levelManager,
                    enemy,
                    start,
                    corner,
                    searchBounds) ||
                !IsBodyPathClear(
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

            NavigationCacheKey key = {
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

        bool IsTileClear(Tile tile) {
            int index = GetIndex(tile);
            if (index < 0) return false;
            std::int8_t& cached = grid->clearTiles[index];
            if (cached >= 0) return cached != 0;

            bool clear = IsBodyClearAtWorldPosition(
                levelManager,
                enemy,
                levelManager.TileToWorld(tile.x, tile.y),
                searchBounds
            );
            cached = clear ? 1 : 0;
            return clear;
        }

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

    private:
        static int Quantize(float value) {
            return (int)std::lround(value * 1000.0f);
        }

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

        int GetIndex(Tile tile) const {
            int localX = tile.x - grid->minimumTileX;
            int localY = tile.y - grid->minimumTileY;
            if (localX < 0 || localY < 0 ||
                localX >= grid->tileWidth || localY >= grid->tileHeight) {
                return -1;
            }
            return localY * grid->tileWidth + localX;
        }

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

            return IsBodyPathClear(
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
        NavigationGridCache* grid = nullptr;
    };

    Tile WorldTile(const LevelManager& levelManager, Vector2 position) {
        Vector2 tile = levelManager.WorldToTile(position);
        return { (int)tile.x, (int)tile.y };
    }

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

    std::optional<PositionConnection> FindStartConnection(
        LevelManager& levelManager,
        Enemy& enemy,
        SearchCollisionCache& cache,
        Rectangle searchBounds
    ) {
        Vector2 start = enemy.GetPosition();
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
                if (!IsBodyClearAtWorldPosition(
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

    std::vector<Vector2> GenerateGoalCandidates(const Paladin& target) {
        constexpr std::array<Vector2, 8> DIRECTIONS = {
            Vector2{ -1.0f, 0.0f },
            Vector2{ 1.0f, 0.0f },
            Vector2{ 0.0f, -1.0f },
            Vector2{ 0.0f, 1.0f },
            Vector2{ -1.0f, -1.0f },
            Vector2{ 1.0f, -1.0f },
            Vector2{ -1.0f, 1.0f },
            Vector2{ 1.0f, 1.0f }
        };

        std::vector<Vector2> candidates;
        candidates.reserve(DIRECTIONS.size());

        for (Vector2 direction : DIRECTIONS) {
            candidates.push_back(Vector2Add(
                target.GetPosition(),
                Vector2Scale(
                    Vector2Normalize(direction),
                    PLAYER_GOAL_RADIUS
                )
            ));
        }

        return candidates;
    }

    bool SameRectangle(Rectangle first, Rectangle second) {
        return std::abs(first.x - second.x) < 0.001f &&
            std::abs(first.y - second.y) < 0.001f &&
            std::abs(first.width - second.width) < 0.001f &&
            std::abs(first.height - second.height) < 0.001f;
    }

    const std::vector<EnemyPathDebugPoint>& GetSharedGoalCandidates(
        LevelManager& levelManager,
        const Paladin& target,
        EnemyNavigationCacheStore& store,
        Rectangle searchBounds
    ) {
        Tile targetTile = WorldTile(levelManager, target.GetPosition());
        if (store.goalsValid &&
            store.goalTargetTileX == targetTile.x &&
            store.goalTargetTileY == targetTile.y &&
            SameRectangle(store.goalSearchBounds, searchBounds)) {
            return store.sharedGoals;
        }

        store.goalsValid = true;
        store.goalTargetTileX = targetTile.x;
        store.goalTargetTileY = targetTile.y;
        store.goalSearchBounds = searchBounds;
        store.sharedGoals.clear();

        std::vector<Vector2> candidates = GenerateGoalCandidates(target);
        store.sharedGoals.reserve(candidates.size());
        for (Vector2 candidate : candidates) {
            Vector2 closestHitboxPoint = ClosestPointOnRectangle(
                candidate,
                target.GetBoundingBox()
            );
            bool insideDomain = CheckCollisionPointRec(
                candidate,
                searchBounds
            );
            bool hasLineOfSight = insideDomain &&
                levelManager.HasClearLineOfSight(
                    candidate,
                    closestHitboxPoint,
                    0.0f
                );
            store.sharedGoals.push_back({ candidate, hasLineOfSight });
        }
        return store.sharedGoals;
    }

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

    std::vector<Vector2> CondenseWaypoints(
        const LevelManager& levelManager,
        const Enemy& enemy,
        const std::vector<Vector2>& rawWaypoints,
        Rectangle searchBounds
    ) {
        std::vector<Vector2> condensed;
        Vector2 cursor = enemy.GetPosition();
        std::size_t index = 0;

        while (index < rawWaypoints.size() &&
               (int)condensed.size() < MAX_TARGET_POSITIONS) {
            std::size_t farthest = index;
            for (std::size_t candidate = rawWaypoints.size();
                 candidate-- > index;) {
                if (IsBodyPathClear(
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
                DistanceSquared(enemy.GetPosition(), candidate.position) <=
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

                result.waypoints = CondenseWaypoints(
                    levelManager,
                    enemy,
                    rawWaypoints,
                    searchBounds
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

    PathSearchResult FindPathToExplicitGoal(
        LevelManager& levelManager,
        Enemy& enemy,
        Vector2 worldGoal,
        EnemyNavigationCacheStore& store,
        Rectangle searchBounds,
        std::uint64_t navigationRevision
    ) {
        const std::vector<EnemyPathDebugPoint> candidates = {
            { worldGoal, true }
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

    bool HasReachedWaypoint(const Enemy& enemy, Vector2 waypoint) {
        Rectangle body = enemy.GetCollisionBox();
        float reachDistance = std::max(
            4.0f,
            std::min(body.width, body.height) * 0.25f
        );
        return DistanceSquared(enemy.GetPosition(), waypoint) <=
            reachDistance * reachDistance;
    }

    void PopReachedTargets(Enemy& enemy) {
        while (enemy.HasTargetPosition() &&
               HasReachedWaypoint(enemy, enemy.FirstTargetPosition())) {
            enemy.PopTarget();
        }
    }
}

EnemyPathManager::EnemyPathManager()
    : navigationCacheStore(
          std::make_unique<EnemyNavigationCacheStore>()) {
}

EnemyPathManager::~EnemyPathManager() = default;

void EnemyPathManager::RemoveEnemy(Enemy& enemy) {
    enemies.erase(
        std::remove(enemies.begin(), enemies.end(), &enemy),
        enemies.end()
    );
    pathRecords.erase(&enemy);

    if (nextEnemyIndex >= (int)enemies.size()) {
        nextEnemyIndex = 0;
    }
    searchCredits = std::min(searchCredits, (float)enemies.size());
}

void EnemyPathManager::AddEnemy(Enemy& enemy) {
    if (std::find(enemies.begin(), enemies.end(), &enemy) == enemies.end()) {
        enemies.push_back(&enemy);
        pathRecords[&enemy] = PathRecord{};
        searchCredits = std::min(
            (float)enemies.size(),
            searchCredits + 1.0f
        );
        enemy.SetPathStatus(EnemyPathStatus::Pending);
    }
}

void EnemyPathManager::Clear() {
    enemies.clear();
    pathRecords.clear();
    nextEnemyIndex = 0;
    searchCredits = 0.0f;
    navigationCacheStore->grids.clear();
    navigationCacheStore->goalsValid = false;
    navigationCacheStore->sharedGoals.clear();
}

void EnemyPathManager::AddEnemyTo(Enemy& enemy, Vector2 worldGoal) {
    AddEnemy(enemy);
    PathRecord& record = pathRecords[&enemy];
    record.hasExplicitGoal = true;
    record.explicitGoal = worldGoal;
    record.hasTargetTile = false;
    record.forceRepath = true;
    record.lastSearchFailed = false;
    record.pathAge = 0.0f;
    enemy.SetPathStatus(EnemyPathStatus::Pending);
}

std::optional<Vector2> EnemyPathManager::GetNextMoveTarget(
    LevelManager& levelManager,
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
        auto record = pathRecords.find(&enemy);
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

Vector2 EnemyPathManager::GetLocalAvoidanceDirection(
    LevelManager& levelManager,
    Enemy& enemy,
    Vector2 desiredDirection
) {
    if (Vector2Length(desiredDirection) <= 0.001f) {
        return { 0.0f, 0.0f };
    }
    if (!enemy.IsEnemyCollisionEnabled()) {
        return Vector2Normalize(desiredDirection);
    }

    Vector2 finalDirection = desiredDirection;
    Vector2 enemyPosition = enemy.GetPosition();
    Rectangle searchBounds = levelManager.GetCurrentRoomBounds();
    constexpr float SEPARATION_RADIUS = 42.0f;
    constexpr float SEPARATION_RADIUS_SQUARED =
        SEPARATION_RADIUS * SEPARATION_RADIUS;
    constexpr float SEPARATION_WEIGHT = 0.85f;

    for (Enemy* otherEnemy : enemies) {
        if (!otherEnemy) continue;
        if (otherEnemy == &enemy ||
            !otherEnemy->IsEnemyCollisionEnabled() ||
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
            finalDirection = Vector2Add(
                finalDirection,
                Vector2Scale(
                    Vector2Normalize(away),
                    strength * SEPARATION_WEIGHT
                )
            );
        }
    }

    constexpr float PROBE_DISTANCE = 18.0f;
    Vector2 forwardProbe = Vector2Add(
        enemyPosition,
        Vector2Scale(desiredDirection, PROBE_DISTANCE)
    );
    if (!IsBodyClearAtWorldPosition(
            levelManager,
            enemy,
            forwardProbe,
            searchBounds)) {
        Vector2 left = { -desiredDirection.y, desiredDirection.x };
        Vector2 right = { desiredDirection.y, -desiredDirection.x };
        bool leftClear = IsBodyClearAtWorldPosition(
            levelManager,
            enemy,
            Vector2Add(enemyPosition, Vector2Scale(left, PROBE_DISTANCE)),
            searchBounds
        );
        bool rightClear = IsBodyClearAtWorldPosition(
            levelManager,
            enemy,
            Vector2Add(enemyPosition, Vector2Scale(right, PROBE_DISTANCE)),
            searchBounds
        );

        if (leftClear && !rightClear) {
            finalDirection = Vector2Add(
                finalDirection,
                Vector2Scale(left, 0.75f)
            );
        } else if (rightClear && !leftClear) {
            finalDirection = Vector2Add(
                finalDirection,
                Vector2Scale(right, 0.75f)
            );
        } else if (leftClear && rightClear) {
            finalDirection = Vector2Add(
                finalDirection,
                Vector2Scale(left, 0.35f)
            );
        }
    }

    return Vector2Length(finalDirection) > 0.001f
        ? Vector2Normalize(finalDirection)
        : desiredDirection;
}

void EnemyPathManager::Update(LevelManager& levelManager, float deltaTime) {
    profilingStats.searchesThisFrame = 0;
    profilingTimer += std::max(0.0f, deltaTime);
    if (profilingTimer >= 1.0f) {
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
        return;
    }

    for (auto& entry : pathRecords) {
        entry.second.pathAge += std::max(0.0f, deltaTime);
    }

    searchCredits += std::max(0.0f, deltaTime) *
        (float)enemies.size() / TARGET_LOOP_ALL_INTERVAL;
    searchCredits = std::clamp(
        searchCredits,
        0.0f,
        (float)enemies.size()
    );

    int availableSearches = std::min(
        (int)std::floor(searchCredits),
        MAX_SEARCHES_PER_FRAME
    );
    if (availableSearches <= 0) return;

    int searchesPerformed = 0;
    int enemiesInspected = 0;
    std::uint64_t navigationRevision =
        levelManager.GetNavigationRevision();
    Rectangle searchBounds = levelManager.GetCurrentRoomBounds();

    while (searchesPerformed < availableSearches &&
           enemiesInspected < (int)enemies.size()) {
        if (nextEnemyIndex >= (int)enemies.size()) nextEnemyIndex = 0;
        Enemy* enemy = enemies[nextEnemyIndex++];
        ++enemiesInspected;
        if (!enemy || enemy->IsDead() || !enemy->IsEnabled()) continue;

        PathRecord& record = pathRecords[enemy];
        Paladin* target = nullptr;
        Vector2 targetPosition = record.explicitGoal;
        if (!record.hasExplicitGoal) {
            TeamManager* targetTeam = enemy->GetTargetTeam();
            target = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
            if (!target) continue;
            targetPosition = target->GetPosition();
        }

        Tile targetTile = WorldTile(levelManager, targetPosition);
        bool targetChanged = !record.hasTargetTile ||
            record.targetTileX != targetTile.x ||
            record.targetTileY != targetTile.y;
        bool navigationChanged =
            record.navigationRevision != navigationRevision;
        bool failedPathRetry = record.lastSearchFailed &&
            record.pathAge >= FAILED_PATH_RETRY_INTERVAL;
        bool needsSearch = record.forceRepath || targetChanged ||
            navigationChanged || failedPathRetry;
        if (!needsSearch) continue;

        PopReachedTargets(*enemy);
        std::chrono::steady_clock::time_point searchStarted;
        if (Constants::DEBUG_SHOW_PATHFINDING_PROFILING) {
            searchStarted = std::chrono::steady_clock::now();
        }
        PathSearchResult result = record.hasExplicitGoal
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

        record.hasTargetTile = true;
        record.targetTileX = targetTile.x;
        record.targetTileY = targetTile.y;
        record.navigationRevision = navigationRevision;
        record.pathAge = 0.0f;
        record.forceRepath = false;
        record.lastSearchFailed =
            result.status == EnemyPathStatus::Unreachable ||
            result.status == EnemyPathStatus::SearchLimitReached;

        enemy->SetPathDebugPoints(std::move(result.debugGoals));
        if (result.status == EnemyPathStatus::Ready) {
            enemy->ClearTargetPosition();
            for (Vector2 waypoint : result.waypoints) {
                enemy->AddTargetPosition(waypoint);
            }
            enemy->SetPathStatus(EnemyPathStatus::Ready);
            enemy->ClearSelectedPathGoal();
            if (result.selectedGoal) {
                enemy->SetSelectedPathGoal(*result.selectedGoal);
            }
            continue;
        }

        if (result.status == EnemyPathStatus::AtGoal) {
            enemy->ClearTargetPosition();
            enemy->SetPathStatus(EnemyPathStatus::AtGoal);
            enemy->ClearSelectedPathGoal();
            if (result.selectedGoal) {
                enemy->SetSelectedPathGoal(*result.selectedGoal);
            }
            continue;
        }

        bool existingPathValid = enemy->HasTargetPosition() &&
            IsBodyPathClear(
                levelManager,
                *enemy,
                enemy->GetPosition(),
                enemy->FirstTargetPosition(),
                searchBounds
            );
        if (existingPathValid) {
            enemy->SetPathStatus(EnemyPathStatus::Ready);
            continue;
        }

        enemy->ClearTargetPosition();
        enemy->ClearSelectedPathGoal();
        enemy->SetPathStatus(result.status);
    }
}
