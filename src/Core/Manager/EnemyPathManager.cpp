#include "Core/Manager/EnemyPathManager.h"
#include "Core/Manager/LevelManager.h"
#include "Entities/Enemy.h"
#include "Entities/PathfindingEnemy.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>

namespace {
    constexpr float DIAGONAL_COST = 1.41421356f;
    constexpr float TARGET_LOOP_ALL_INTERVAL = 0.2f;
    constexpr int MAX_SEARCH_STEPS = 500;
    constexpr int MIN_TARGET_POSITIONS = 2;
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

    struct AStarNode {
        Tile tile;
        float gCost;
        float fCost;
    };

    struct CompareAStarNode {
        bool operator()(const AStarNode& a, const AStarNode& b) const {
            return a.fCost > b.fCost;
        }
    };

    float Heuristic(Tile a, Tile b) {
        int dx = std::abs(a.x - b.x);
        int dy = std::abs(a.y - b.y);
        int straightSteps = std::abs(dx - dy);
        int diagonalSteps = std::min(dx, dy);

        return (float)straightSteps + DIAGONAL_COST * (float)diagonalSteps;
    }

    Rectangle GetBodyAtWorldPosition(
        PathfindingEnemy* enemy,
        Vector2 worldPosition
    ) {
        Rectangle currentBox = enemy->GetBoundingBox();
        Vector2 currentPosition = enemy->GetPosition();

        return {
            worldPosition.x - (currentPosition.x - currentBox.x),
            worldPosition.y - (currentPosition.y - currentBox.y),
            currentBox.width,
            currentBox.height
        };
    }

    bool IsBodyClearAtWorldPosition(
        LevelManager* levelManager,
        PathfindingEnemy* enemy,
        Vector2 worldPosition
    ) {
        if (!levelManager || !enemy) return false;

        return !levelManager->IsSolidCollision(GetBodyAtWorldPosition(enemy, worldPosition));
    }

    bool IsBodyClearAtTile(
        LevelManager* levelManager,
        PathfindingEnemy* enemy,
        Tile tile
    ) {
        if (!levelManager || !enemy) return false;

        Vector2 worldPosition = levelManager->TileToWorld(tile.x, tile.y);
        return IsBodyClearAtWorldPosition(levelManager, enemy, worldPosition);
    }

    bool CanMoveBetweenTiles(
        LevelManager* levelManager,
        PathfindingEnemy* enemy,
        Tile current,
        Tile neighbor
    ) {
        int dx = neighbor.x - current.x;
        int dy = neighbor.y - current.y;

        if (!IsBodyClearAtTile(levelManager, enemy, neighbor)) {
            return false;
        }

        if (dx != 0 && dy != 0) {
            Tile horizontalTile = { current.x + dx, current.y };
            Tile verticalTile = { current.x, current.y + dy };

            if (!IsBodyClearAtTile(levelManager, enemy, horizontalTile)) {
                return false;
            }

            if (!IsBodyClearAtTile(levelManager, enemy, verticalTile)) {
                return false;
            }
        }

        return true;
    }

    bool HasReachedWaypoint(PathfindingEnemy* enemy, Vector2 waypoint) {
        Rectangle body = enemy->GetBoundingBox();
        float reachDistance = std::max(4.0f, std::min(body.width, body.height) * 0.25f);

        Vector2 position = enemy->GetPosition();
        float dx = waypoint.x - position.x;
        float dy = waypoint.y - position.y;

        return (dx * dx + dy * dy) <= reachDistance * reachDistance;
    }

    void PopReachedTargets(PathfindingEnemy* enemy) {
        while (enemy->HasTargetPosition() &&
               HasReachedWaypoint(enemy, enemy->FirstTargetPosition())) {
            enemy->PopTarget();
        }
    }

    bool SameTileDirection(Tile a, Tile b, Tile c) {
        int dx1 = b.x - a.x;
        int dy1 = b.y - a.y;
        int dx2 = c.x - b.x;
        int dy2 = c.y - b.y;

        return dx1 == dx2 && dy1 == dy2;
    }

    bool AlmostSamePosition(Vector2 a, Vector2 b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return (dx * dx + dy * dy) < 4.0f;
    }

    void PushTarget(std::vector<Vector2>& targets, Vector2 position) {
        if ((int)targets.size() >= MAX_TARGET_POSITIONS) return;
        if (!targets.empty() && AlmostSamePosition(targets.back(), position)) return;

        targets.push_back(position);
    }

    std::vector<Vector2> BuildTargetPositions(
        LevelManager* levelManager,
        PathfindingEnemy* enemy,
        const std::vector<Tile>& path
    ) {
        std::vector<Vector2> targets;
        if (!levelManager || !enemy || path.size() < 2) {
            return targets;
        }

        constexpr int FINAL_TARGET_SLOT_COUNT = 1;
        int turnLimit = MAX_TARGET_POSITIONS - FINAL_TARGET_SLOT_COUNT;

        for (int i = 1; i + 1 < (int)path.size() && (int)targets.size() < turnLimit; ++i) {
            if (!SameTileDirection(path[i - 1], path[i], path[i + 1])) {
                PushTarget(targets, levelManager->TileToWorld(path[i].x, path[i].y));
            }
        }

        if ((int)targets.size() < MIN_TARGET_POSITIONS - 1 && path.size() > 2) {
            Tile nearPlayerTile = path[path.size() - 2];
            PushTarget(targets, levelManager->TileToWorld(nearPlayerTile.x, nearPlayerTile.y));
        }

        PushTarget(targets, enemy->GetTargetTeam()->GetActivePaladin()->GetPosition());
        return targets;
    }

    std::vector<Tile> FindPath(
        LevelManager* levelManager,
        PathfindingEnemy* enemy,
        Tile start,
        Tile goal
    ) {
        if (!levelManager) return {};
        if (!enemy) return {};
        if (!IsBodyClearAtTile(levelManager, enemy, start)) return {};
        if (!IsBodyClearAtTile(levelManager, enemy, goal)) return {};

        std::priority_queue<AStarNode, std::vector<AStarNode>, CompareAStarNode> openSet;
        std::unordered_map<Tile, Tile, TileHash> cameFrom;
        std::unordered_map<Tile, float, TileHash> gScore;
        cameFrom.reserve(MAX_SEARCH_STEPS);
        gScore.reserve(MAX_SEARCH_STEPS);

        openSet.push({ start, 0.0f, Heuristic(start, goal) });
        gScore[start] = 0.0f;

        constexpr Tile directions[8] = {
            { 1, 0 },
            { -1, 0 },
            { 0, 1 },
            { 0, -1 },
            { 1, 1 },
            { 1, -1 },
            { -1, 1 },
            { -1, -1 }
        };

        int searchSteps = 0;
        while (!openSet.empty() && searchSteps < MAX_SEARCH_STEPS) {
            AStarNode current = openSet.top();
            openSet.pop();
            searchSteps++;

            auto currentScore = gScore.find(current.tile);
            if (currentScore != gScore.end() && current.gCost > currentScore->second) {
                continue;
            }

            if (current.tile == goal) {
                std::vector<Tile> path;
                Tile step = goal;
                path.push_back(step);

                while (!(step == start)) {
                    step = cameFrom[step];
                    path.push_back(step);
                }

                std::reverse(path.begin(), path.end());
                return path;
            }

            for (Tile direction : directions) {
                Tile neighbor = {
                    current.tile.x + direction.x,
                    current.tile.y + direction.y
                };

                if (!CanMoveBetweenTiles(levelManager, enemy, current.tile, neighbor)) {
                    continue;
                }

                bool isDiagonal = direction.x != 0 && direction.y != 0;
                float moveCost = isDiagonal ? DIAGONAL_COST : 1.0f;
                float tentativeGCost = current.gCost + moveCost;
                auto existingScore = gScore.find(neighbor);

                if (existingScore == gScore.end() || tentativeGCost < existingScore->second) {
                    cameFrom[neighbor] = current.tile;
                    gScore[neighbor] = tentativeGCost;

                    float fCost = tentativeGCost + Heuristic(neighbor, goal);
                    openSet.push({ neighbor, tentativeGCost, fCost });
                }
            }
        }

        return {};
    }
}

void EnemyPathManager::RemoveEnemy(PathfindingEnemy* enemy) {
    enemies.erase(
        std::remove(enemies.begin(), enemies.end(), enemy),
        enemies.end()
    );

    if (nextEnemyIndex >= (int)enemies.size()) {
        nextEnemyIndex = 0;
    }
}

void EnemyPathManager::AddEnemy(PathfindingEnemy* enemy) {
    if (std::find(enemies.begin(), enemies.end(), enemy) == enemies.end()) {
        enemies.push_back(enemy);
    }
}

void EnemyPathManager::Clear() {
    enemies.clear();
    nextEnemyIndex = 0;
}

Vector2 EnemyPathManager::GetNextMoveTarget(
    LevelManager* levelManager,
    PathfindingEnemy* enemy,
    Vector2 fallbackTarget
) {
    if (!levelManager || !enemy) {
        return fallbackTarget;
    }

    PopReachedTargets(enemy);

    if (enemy->HasTargetPosition()) {
        Vector2 targetPosition = enemy->FirstTargetPosition();
        if (IsBodyClearAtWorldPosition(levelManager, enemy, targetPosition)) {
            return targetPosition;
        }

        enemy->ClearTargetPosition();
    }

    return fallbackTarget;
}

Vector2 EnemyPathManager::GetLocalAvoidanceDirection(
    LevelManager* levelManager,
    PathfindingEnemy* enemy,
    Vector2 desiredDirection
) {
    if (!levelManager || !enemy) {
        return desiredDirection;
    }

    Vector2 finalDirection = desiredDirection;
    Vector2 enemyPosition = enemy->GetPosition();
    const float separationRadius = 42.0f;
    const float separationWeight = 0.85f;

    for (GameObject* entity : levelManager->GetEntities()) {
        Enemy* otherEnemy = static_cast<Enemy*>(entity);
        if (!otherEnemy || otherEnemy == enemy || otherEnemy->IsDead()) {
            continue;
        }

        Vector2 away = Vector2Subtract(enemyPosition, otherEnemy->GetPosition());
        float distance = Vector2Length(away);
        if (distance > 0.001f && distance < separationRadius) {
            float strength = (separationRadius - distance) / separationRadius;
            finalDirection = Vector2Add(
                finalDirection,
                Vector2Scale(Vector2Normalize(away), strength * separationWeight)
            );
        }
    }

    if (Vector2Length(desiredDirection) > 0.001f) {
        const float probeDistance = 18.0f;
        Vector2 forwardProbe = Vector2Add(enemyPosition, Vector2Scale(desiredDirection, probeDistance));

        if (!IsBodyClearAtWorldPosition(levelManager, enemy, forwardProbe)) {
            Vector2 left = { -desiredDirection.y, desiredDirection.x };
            Vector2 right = { desiredDirection.y, -desiredDirection.x };
            Vector2 leftProbe = Vector2Add(enemyPosition, Vector2Scale(left, probeDistance));
            Vector2 rightProbe = Vector2Add(enemyPosition, Vector2Scale(right, probeDistance));

            bool leftClear = IsBodyClearAtWorldPosition(levelManager, enemy, leftProbe);
            bool rightClear = IsBodyClearAtWorldPosition(levelManager, enemy, rightProbe);

            if (leftClear && !rightClear) {
                finalDirection = Vector2Add(finalDirection, Vector2Scale(left, 0.75f));
            } else if (rightClear && !leftClear) {
                finalDirection = Vector2Add(finalDirection, Vector2Scale(right, 0.75f));
            } else if (leftClear && rightClear) {
                finalDirection = Vector2Add(finalDirection, Vector2Scale(left, 0.35f));
            }
        }
    }

    if (Vector2Length(finalDirection) > 0.001f) {
        return Vector2Normalize(finalDirection);
    }

    return desiredDirection;
}

void EnemyPathManager::Update(LevelManager* levelManager, float deltaTime) {
    if (!levelManager || enemies.empty()) return;
    if (TARGET_LOOP_ALL_INTERVAL <= 0.0f) return;

    int enemiesPerFrame = (int)std::ceil(
        (float)enemies.size() *
        std::min(1.0f, deltaTime / TARGET_LOOP_ALL_INTERVAL)
    );
    if (enemiesPerFrame < 1) {
        enemiesPerFrame = 1;
    }

    for (int i = 0; i < enemiesPerFrame; ++i) {
        if (enemies.empty()) {
            return;
        }

        if (nextEnemyIndex >= (int)enemies.size()) {
            nextEnemyIndex = 0;
        }

        PathfindingEnemy* enemy = enemies[nextEnemyIndex];
        nextEnemyIndex++;

        if (!enemy || enemy->IsDead() || !enemy->GetTargetTeam()) {
            continue;
        }

        PopReachedTargets(enemy);

        Vector2 enemyTilePosition = levelManager->WorldToTile(enemy->GetPosition());
        Vector2 playerTilePosition = levelManager->WorldToTile(enemy->GetTargetTeam()->GetActivePaladin()->GetPosition());

        Tile enemyTile = {
            (int)enemyTilePosition.x,
            (int)enemyTilePosition.y
        };

        Tile playerTile = {
            (int)playerTilePosition.x,
            (int)playerTilePosition.y
        };

        std::vector<Tile> path = FindPath(levelManager, enemy, enemyTile, playerTile);
        if (path.size() < 2) {
            enemy->ClearTargetPosition();
            continue;
        }

        std::vector<Vector2> targets = BuildTargetPositions(levelManager, enemy, path);
        if ((int)targets.size() < MIN_TARGET_POSITIONS) {
            Vector2 firstMove = levelManager->TileToWorld(path[1].x, path[1].y);
            if (targets.empty() || !AlmostSamePosition(firstMove, targets.front())) {
                targets.insert(targets.begin(), firstMove);
            }
        }

        if ((int)targets.size() > MAX_TARGET_POSITIONS) {
            targets.resize(MAX_TARGET_POSITIONS);
        }

        enemy->ClearTargetPosition();
        for (Vector2 targetPosition : targets) {
            enemy->AddTargetPosition(targetPosition);
        }
    }
}
