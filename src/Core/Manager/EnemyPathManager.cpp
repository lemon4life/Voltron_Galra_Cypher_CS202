#include "Core/Manager/EnemyPathManager.h"

#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"

#include "raymath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>

namespace {
    constexpr float DIAGONAL_COST = 1.41421356f;
    constexpr float TARGET_LOOP_ALL_INTERVAL = 0.2f;
    constexpr float BODY_PATH_SAMPLE_SPACING = 4.0f;
    constexpr float POSITION_EPSILON_SQUARED = 4.0f;
    constexpr float BODY_GOAL_OVERLAP = 1.0f;
    constexpr float QUARTER_TILE_OFFSET = 8.0f;
    constexpr int MAX_SEARCH_STEPS = 500;
    constexpr int MAX_TARGET_POSITIONS = 10;

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

    bool TileLess(Tile first, Tile second) {
        return first.x < second.x ||
            (first.x == second.x && first.y < second.y);
    }

    struct TileEdge {
        Tile first;
        Tile second;

        bool operator==(const TileEdge& other) const {
            return first == other.first && second == other.second;
        }
    };

    struct TileEdgeHash {
        std::size_t operator()(const TileEdge& edge) const {
            TileHash hash;
            return hash(edge.first) ^ (hash(edge.second) << 1);
        }
    };

    TileEdge MakeTileEdge(Tile first, Tile second) {
        if (TileLess(second, first)) {
            std::swap(first, second);
        }
        return { first, second };
    }

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
    };

    float DistanceSquared(Vector2 first, Vector2 second) {
        float dx = first.x - second.x;
        float dy = first.y - second.y;
        return dx * dx + dy * dy;
    }

    bool AlmostSamePosition(Vector2 first, Vector2 second) {
        return DistanceSquared(first, second) < POSITION_EPSILON_SQUARED;
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
        Vector2 worldPosition
    ) {
        Rectangle footprint = enemy.GetNavigationFootprintAt(worldPosition);
        return ContainsRectangle(levelManager.GetLevelBounds(), footprint) &&
            !levelManager.IsSolidCollision(footprint);
    }

    bool IsBodyPathClear(
        const LevelManager& levelManager,
        const Enemy& enemy,
        Vector2 start,
        Vector2 end
    ) {
        float distance = Vector2Distance(start, end);
        int sampleCount = std::max(
            1,
            (int)std::ceil(distance / BODY_PATH_SAMPLE_SPACING)
        );

        for (int sample = 0; sample <= sampleCount; ++sample) {
            float amount = (float)sample / (float)sampleCount;
            Vector2 position = Vector2Lerp(start, end, amount);
            if (!IsBodyClearAtWorldPosition(levelManager, enemy, position)) {
                return false;
            }
        }

        return true;
    }

    std::optional<std::vector<Vector2>> ConnectPositions(
        const LevelManager& levelManager,
        const Enemy& enemy,
        Vector2 start,
        Vector2 end
    ) {
        if (IsBodyPathClear(levelManager, enemy, start, end)) {
            return std::vector<Vector2>{ end };
        }

        std::array<Vector2, 2> corners = {
            Vector2{ end.x, start.y },
            Vector2{ start.x, end.y }
        };
        std::optional<std::vector<Vector2>> best;
        float bestLength = std::numeric_limits<float>::max();

        for (Vector2 corner : corners) {
            if (!IsBodyClearAtWorldPosition(levelManager, enemy, corner) ||
                !IsBodyPathClear(levelManager, enemy, start, corner) ||
                !IsBodyPathClear(levelManager, enemy, corner, end)) {
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
        SearchCollisionCache(LevelManager& levelManager, Enemy& enemy)
            : levelManager(levelManager), enemy(enemy) {}

        bool IsTileClear(Tile tile) {
            auto existing = tileClear.find(tile);
            if (existing != tileClear.end()) {
                return existing->second;
            }

            bool clear = IsBodyClearAtWorldPosition(
                levelManager,
                enemy,
                levelManager.TileToWorld(tile.x, tile.y)
            );
            tileClear[tile] = clear;
            return clear;
        }

        bool IsEdgeClear(Tile current, Tile neighbor) {
            TileEdge edge = MakeTileEdge(current, neighbor);
            auto existing = edgeClear.find(edge);
            if (existing != edgeClear.end()) {
                return existing->second;
            }

            bool clear = CalculateEdgeClear(current, neighbor);
            edgeClear[edge] = clear;
            return clear;
        }

    private:
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
                levelManager.TileToWorld(neighbor.x, neighbor.y)
            );
        }

        LevelManager& levelManager;
        Enemy& enemy;
        std::unordered_map<Tile, bool, TileHash> tileClear;
        std::unordered_map<TileEdge, bool, TileEdgeHash> edgeClear;
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
        SearchCollisionCache& cache
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
        for (Tile graphTile : graphTiles) {
            if (!cache.IsTileClear(graphTile)) continue;
            Vector2 center = levelManager.TileToWorld(graphTile.x, graphTile.y);
            auto connection = ConnectPositions(levelManager, enemy, start, center);
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

        for (Tile anchorTile : graphTiles) {
            Vector2 tileCenter = levelManager.TileToWorld(
                anchorTile.x,
                anchorTile.y
            );
            for (Vector2 offset : ANCHOR_OFFSETS) {
                Vector2 anchor = Vector2Add(tileCenter, offset);
                if (!IsBodyClearAtWorldPosition(levelManager, enemy, anchor)) {
                    continue;
                }

                auto toAnchor = ConnectPositions(
                    levelManager,
                    enemy,
                    start,
                    anchor
                );
                if (!toAnchor) continue;

                for (Tile graphTile : graphTiles) {
                    if (!cache.IsTileClear(graphTile)) continue;
                    Vector2 graphCenter = levelManager.TileToWorld(
                        graphTile.x,
                        graphTile.y
                    );
                    auto toGraph = ConnectPositions(
                        levelManager,
                        enemy,
                        anchor,
                        graphCenter
                    );
                    if (!toGraph) continue;

                    std::vector<Vector2> points = *toAnchor;
                    points.insert(points.end(), toGraph->begin(), toGraph->end());
                    consider(graphTile, std::move(points));
                }
            }
        }

        return best;
    }

    std::vector<Vector2> GenerateGoalCandidates(
        const Enemy& enemy,
        const Paladin& target
    ) {
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
        float preferredDistance = enemy.GetPreferredPathGoalDistance();

        if (preferredDistance > 0.0f) {
            for (Vector2 direction : DIRECTIONS) {
                Vector2 normalized = Vector2Normalize(direction);
                candidates.push_back(Vector2Add(
                    target.GetPosition(),
                    Vector2Scale(normalized, preferredDistance)
                ));
            }
            return candidates;
        }

        Rectangle targetBody = target.GetCollisionBox();
        Rectangle footprintAtOrigin = enemy.GetNavigationFootprintAt({ 0.0f, 0.0f });
        Vector2 footprintCenterOffset = {
            footprintAtOrigin.x + footprintAtOrigin.width / 2.0f,
            footprintAtOrigin.y + footprintAtOrigin.height / 2.0f
        };
        Vector2 targetCenter = {
            targetBody.x + targetBody.width / 2.0f,
            targetBody.y + targetBody.height / 2.0f
        };

        for (Vector2 direction : DIRECTIONS) {
            Vector2 footprintCenter = targetCenter;
            if (direction.x != 0.0f) {
                footprintCenter.x += direction.x * (
                    targetBody.width / 2.0f +
                    footprintAtOrigin.width / 2.0f -
                    BODY_GOAL_OVERLAP
                );
            }
            if (direction.y != 0.0f) {
                footprintCenter.y += direction.y * (
                    targetBody.height / 2.0f +
                    footprintAtOrigin.height / 2.0f -
                    BODY_GOAL_OVERLAP
                );
            }

            candidates.push_back(Vector2Subtract(
                footprintCenter,
                footprintCenterOffset
            ));
        }

        return candidates;
    }

    std::vector<GoalAnchor> BuildGoalAnchors(
        LevelManager& levelManager,
        Enemy& enemy,
        const Paladin& target,
        SearchCollisionCache& cache,
        std::vector<EnemyPathDebugPoint>& debugGoals
    ) {
        std::vector<GoalAnchor> anchors;
        std::vector<Vector2> candidates = GenerateGoalCandidates(enemy, target);
        debugGoals.reserve(candidates.size());

        for (Vector2 candidate : candidates) {
            bool validCandidate =
                IsBodyClearAtWorldPosition(levelManager, enemy, candidate) &&
                enemy.IsValidPathGoalPosition(candidate, target);
            bool hasAnchor = false;

            if (validCandidate) {
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
                        candidate
                    );
                    if (!connection) continue;

                    anchors.push_back({
                        graphTile,
                        candidate,
                        *connection,
                        ConnectionLength(graphCenter, *connection)
                    });
                    hasAnchor = true;
                }
            }

            debugGoals.push_back({ candidate, validCandidate && hasAnchor });
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
        const std::vector<Vector2>& rawWaypoints
    ) {
        std::vector<Vector2> condensed;
        Vector2 cursor = enemy.GetPosition();
        std::size_t index = 0;

        while (index < rawWaypoints.size() &&
               (int)condensed.size() < MAX_TARGET_POSITIONS) {
            std::size_t farthest = index;
            for (std::size_t candidate = index;
                 candidate < rawWaypoints.size();
                 ++candidate) {
                if (IsBodyPathClear(
                        levelManager,
                        enemy,
                        cursor,
                        rawWaypoints[candidate])) {
                    farthest = candidate;
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

    PathSearchResult FindPath(
        LevelManager& levelManager,
        Enemy& enemy,
        Paladin& target
    ) {
        PathSearchResult result;

        if (!IsBodyClearAtWorldPosition(
                levelManager,
                enemy,
                enemy.GetPosition())) {
            return result;
        }

        if (enemy.IsValidPathGoalPosition(enemy.GetPosition(), target)) {
            result.status = EnemyPathStatus::AtGoal;
            result.selectedGoal = enemy.GetPosition();
            return result;
        }

        SearchCollisionCache cache(levelManager, enemy);
        std::optional<PositionConnection> start = FindStartConnection(
            levelManager,
            enemy,
            cache
        );
        std::vector<GoalAnchor> goals = BuildGoalAnchors(
            levelManager,
            enemy,
            target,
            cache,
            result.debugGoals
        );
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
                    rawWaypoints
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
        return result;
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

void EnemyPathManager::RemoveEnemy(Enemy& enemy) {
    enemies.erase(
        std::remove(enemies.begin(), enemies.end(), &enemy),
        enemies.end()
    );

    if (nextEnemyIndex >= (int)enemies.size()) {
        nextEnemyIndex = 0;
    }
}

void EnemyPathManager::AddEnemy(Enemy& enemy) {
    if (std::find(enemies.begin(), enemies.end(), &enemy) == enemies.end()) {
        enemies.push_back(&enemy);
        enemy.SetPathStatus(EnemyPathStatus::Pending);
    }
}

void EnemyPathManager::Clear() {
    enemies.clear();
    nextEnemyIndex = 0;
}

std::optional<Vector2> EnemyPathManager::GetNextMoveTarget(
    LevelManager& levelManager,
    Enemy& enemy
) {
    PopReachedTargets(enemy);

    if (!enemy.HasTargetPosition()) {
        return std::nullopt;
    }

    Vector2 targetPosition = enemy.FirstTargetPosition();
    if (!IsBodyClearAtWorldPosition(levelManager, enemy, targetPosition)) {
        enemy.ClearTargetPosition();
        enemy.ClearSelectedPathGoal();
        enemy.SetPathStatus(EnemyPathStatus::Pending);
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

    Vector2 finalDirection = desiredDirection;
    Vector2 enemyPosition = enemy.GetPosition();
    constexpr float SEPARATION_RADIUS = 42.0f;
    constexpr float SEPARATION_WEIGHT = 0.85f;

    for (GameObject* entity : levelManager.GetEntities()) {
        if (entity->GetObjectType() != GameObjectType::Enemy) continue;

        Enemy* otherEnemy = static_cast<Enemy*>(entity);
        if (otherEnemy == &enemy || otherEnemy->IsDead()) continue;

        Vector2 away = Vector2Subtract(
            enemyPosition,
            otherEnemy->GetPosition()
        );
        float distance = Vector2Length(away);
        if (distance > 0.001f && distance < SEPARATION_RADIUS) {
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
    if (!IsBodyClearAtWorldPosition(levelManager, enemy, forwardProbe)) {
        Vector2 left = { -desiredDirection.y, desiredDirection.x };
        Vector2 right = { desiredDirection.y, -desiredDirection.x };
        bool leftClear = IsBodyClearAtWorldPosition(
            levelManager,
            enemy,
            Vector2Add(enemyPosition, Vector2Scale(left, PROBE_DISTANCE))
        );
        bool rightClear = IsBodyClearAtWorldPosition(
            levelManager,
            enemy,
            Vector2Add(enemyPosition, Vector2Scale(right, PROBE_DISTANCE))
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
    if (enemies.empty() || TARGET_LOOP_ALL_INTERVAL <= 0.0f) return;

    int enemiesPerFrame = (int)std::ceil(
        (float)enemies.size() *
        std::min(1.0f, deltaTime / TARGET_LOOP_ALL_INTERVAL)
    );
    enemiesPerFrame = std::max(1, enemiesPerFrame);

    for (int index = 0; index < enemiesPerFrame; ++index) {
        if (enemies.empty()) return;
        if (nextEnemyIndex >= (int)enemies.size()) nextEnemyIndex = 0;

        Enemy* enemy = enemies[nextEnemyIndex++];
        if (!enemy || enemy->IsDead() || !enemy->GetTargetTeam()) continue;

        Paladin* target = enemy->GetTargetTeam()->GetActivePaladin();
        if (!target) continue;

        PopReachedTargets(*enemy);
        PathSearchResult result = FindPath(levelManager, *enemy, *target);

        enemy->ClearTargetPosition();
        enemy->SetPathStatus(result.status);
        enemy->SetPathDebugPoints(std::move(result.debugGoals));
        enemy->ClearSelectedPathGoal();
        if (result.selectedGoal) {
            enemy->SetSelectedPathGoal(*result.selectedGoal);
        }

        if (result.status != EnemyPathStatus::Ready) continue;
        for (Vector2 waypoint : result.waypoints) {
            enemy->AddTargetPosition(waypoint);
        }
    }
}
