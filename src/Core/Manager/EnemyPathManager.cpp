#include "Core/Manager/EnemyPathManager.h"
#include "Core/Manager/LevelManager.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/TeamManager.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace {
    constexpr float DIAGONAL_COST = 1.41421356f;
    constexpr float TARGET_LOOP_ALL_INTERVAL = 0.2f;
    constexpr int MAX_SEARCH_STEPS = 500;
    constexpr float BODY_PATH_PROBE_SPACING = 4.0f;

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

    struct PathSearchResult {
        EnemyPathStatus status = EnemyPathStatus::Unreachable;
        std::vector<Vector2> targets;
    };

    float Heuristic(Tile a, Tile b) {
        int dx = std::abs(a.x - b.x);
        int dy = std::abs(a.y - b.y);
        int straightSteps = std::abs(dx - dy);
        int diagonalSteps = std::min(dx, dy);

        return (float)straightSteps + DIAGONAL_COST * (float)diagonalSteps;
    }

    Rectangle GetBodyAtWorldPosition(
        Enemy* enemy,
        Vector2 worldPosition
    ) {
        Rectangle currentBox = enemy->GetCollisionBox();
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
        Enemy* enemy,
        Vector2 worldPosition
    ) {
        if (!levelManager || !enemy) return false;

        return !levelManager->IsSolidCollision(
            GetBodyAtWorldPosition(enemy, worldPosition)
        );
    }

    bool IsBodyClearAtTile(
        LevelManager* levelManager,
        Enemy* enemy,
        Tile tile
    ) {
        if (!levelManager || !enemy) return false;

        Vector2 worldPosition = levelManager->TileToWorld(tile.x, tile.y);
        return IsBodyClearAtWorldPosition(
            levelManager,
            enemy,
            worldPosition
        );
    }

    bool IsBodyPathClear(
        LevelManager* levelManager,
        Enemy* enemy,
        Vector2 start,
        Vector2 end
    ) {
        if (!levelManager || !enemy) return false;

        Vector2 segment = Vector2Subtract(end, start);
        float distance = Vector2Length(segment);
        int probeCount = std::max(
            1,
            (int)std::ceil(distance / BODY_PATH_PROBE_SPACING)
        );

        for (int probeIndex = 0; probeIndex <= probeCount; ++probeIndex) {
            float amount = (float)probeIndex / (float)probeCount;
            Vector2 probePosition = Vector2Add(
                start,
                Vector2Scale(segment, amount)
            );
            if (!IsBodyClearAtWorldPosition(
                    levelManager,
                    enemy,
                    probePosition
                )) {
                return false;
            }
        }

        return true;
    }

    bool CanMoveBetweenTiles(
        LevelManager* levelManager,
        Enemy* enemy,
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

            if (!IsBodyClearAtTile(levelManager, enemy, horizontalTile) ||
                !IsBodyClearAtTile(levelManager, enemy, verticalTile)) {
                return false;
            }
        }

        return true;
    }

    bool HasReachedWaypoint(Enemy* enemy, Vector2 waypoint) {
        Rectangle body = enemy->GetCollisionBox();
        float reachDistance = std::max(
            4.0f,
            std::min(body.width, body.height) * 0.25f
        );

        Vector2 position = enemy->GetPosition();
        float dx = waypoint.x - position.x;
        float dy = waypoint.y - position.y;

        return (dx * dx + dy * dy) <= reachDistance * reachDistance;
    }

    void PopReachedTargets(Enemy* enemy) {
        while (enemy->HasTargetPosition() &&
               HasReachedWaypoint(enemy, enemy->FirstTargetPosition())) {
            enemy->PopTarget();
        }
    }

    bool AlmostSamePosition(Vector2 a, Vector2 b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return (dx * dx + dy * dy) < 4.0f;
    }

    bool IsGoalPosition(
        LevelManager* levelManager,
        Enemy* enemy,
        Vector2 candidatePosition,
        Vector2 playerPosition
    ) {
        if (!levelManager || !enemy) return false;

        EnemyPathGoal goal = enemy->GetPathGoal();
        if (Vector2Distance(candidatePosition, playerPosition) >
            goal.acceptanceDistance) {
            return false;
        }

        if (goal.mode == EnemyPathGoalMode::ClearLineOfSight) {
            return levelManager->HasClearLineOfSight(
                candidatePosition,
                playerPosition,
                goal.clearanceRadius
            );
        }

        return IsBodyPathClear(
            levelManager,
            enemy,
            candidatePosition,
            playerPosition
        );
    }

    std::vector<Vector2> CondenseValidatedRoute(
        LevelManager* levelManager,
        Enemy* enemy,
        const std::vector<Vector2>& fullRoute
    ) {
        std::vector<Vector2> targets;
        if (!levelManager || !enemy) return targets;

        Vector2 segmentStart = enemy->GetPosition();
        size_t nextIndex = 0;
        while (nextIndex < fullRoute.size()) {
            size_t selectedIndex = fullRoute.size();
            for (size_t candidateIndex = fullRoute.size();
                 candidateIndex > nextIndex;
                 --candidateIndex) {
                size_t routeIndex = candidateIndex - 1;
                if (IsBodyPathClear(
                        levelManager,
                        enemy,
                        segmentStart,
                        fullRoute[routeIndex]
                    )) {
                    selectedIndex = routeIndex;
                    break;
                }
            }

            if (selectedIndex == fullRoute.size()) {
                return {};
            }

            Vector2 selectedTarget = fullRoute[selectedIndex];
            if (!AlmostSamePosition(segmentStart, selectedTarget)) {
                targets.push_back(selectedTarget);
            }
            segmentStart = selectedTarget;
            nextIndex = selectedIndex + 1;
        }

        return targets;
    }

    PathSearchResult FindPath(
        LevelManager* levelManager,
        Enemy* enemy,
        Vector2 playerPosition
    ) {
        if (!levelManager || !enemy) return {};

        Vector2 enemyPosition = enemy->GetPosition();
        if (IsGoalPosition(
                levelManager,
                enemy,
                enemyPosition,
                playerPosition
            )) {
            return { EnemyPathStatus::AtGoal, {} };
        }

        Vector2 enemyTilePosition = levelManager->WorldToTile(enemyPosition);
        Vector2 playerTilePosition = levelManager->WorldToTile(playerPosition);
        Tile enemyTile = {
            (int)enemyTilePosition.x,
            (int)enemyTilePosition.y
        };
        Tile playerTile = {
            (int)playerTilePosition.x,
            (int)playerTilePosition.y
        };

        std::priority_queue<
            AStarNode,
            std::vector<AStarNode>,
            CompareAStarNode
        > openSet;
        std::unordered_map<Tile, Tile, TileHash> cameFrom;
        std::unordered_map<Tile, float, TileHash> gScore;
        std::unordered_set<Tile, TileHash> closedSet;
        cameFrom.reserve(MAX_SEARCH_STEPS);
        gScore.reserve(MAX_SEARCH_STEPS);
        closedSet.reserve(MAX_SEARCH_STEPS);

        for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
            for (int columnOffset = -1; columnOffset <= 1; ++columnOffset) {
                Tile startTile = {
                    enemyTile.x + columnOffset,
                    enemyTile.y + rowOffset
                };
                Vector2 startPosition = levelManager->TileToWorld(
                    startTile.x,
                    startTile.y
                );
                if (!IsBodyClearAtTile(levelManager, enemy, startTile) ||
                    !IsBodyPathClear(
                        levelManager,
                        enemy,
                        enemyPosition,
                        startPosition
                    )) {
                    continue;
                }

                float startCost = Vector2Distance(
                    enemyPosition,
                    startPosition
                ) / 32.0f;
                auto existingStart = gScore.find(startTile);
                if (existingStart != gScore.end() &&
                    existingStart->second <= startCost) {
                    continue;
                }

                gScore[startTile] = startCost;
                openSet.push({
                    startTile,
                    startCost,
                    startCost + Heuristic(startTile, playerTile)
                });
            }
        }

        if (openSet.empty()) {
            return { EnemyPathStatus::Unreachable, {} };
        }

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

            auto currentScore = gScore.find(current.tile);
            if (currentScore != gScore.end() &&
                current.gCost > currentScore->second) {
                continue;
            }

            if (closedSet.find(current.tile) != closedSet.end()) {
                continue;
            }
            closedSet.insert(current.tile);
            searchSteps++;

            Vector2 currentPosition = levelManager->TileToWorld(
                current.tile.x,
                current.tile.y
            );
            if (IsGoalPosition(
                    levelManager,
                    enemy,
                    currentPosition,
                    playerPosition
                )) {
                std::vector<Tile> path;
                Tile step = current.tile;
                path.push_back(step);

                auto parent = cameFrom.find(step);
                while (parent != cameFrom.end()) {
                    step = parent->second;
                    path.push_back(step);
                    parent = cameFrom.find(step);
                }
                std::reverse(path.begin(), path.end());

                std::vector<Vector2> fullRoute;
                fullRoute.reserve(path.size() + 1);
                for (Tile pathTile : path) {
                    fullRoute.push_back(levelManager->TileToWorld(
                        pathTile.x,
                        pathTile.y
                    ));
                }

                if (enemy->GetPathGoal().mode ==
                        EnemyPathGoalMode::BodyApproach &&
                    (fullRoute.empty() ||
                     !AlmostSamePosition(fullRoute.back(), playerPosition))) {
                    fullRoute.push_back(playerPosition);
                }

                std::vector<Vector2> targets = CondenseValidatedRoute(
                    levelManager,
                    enemy,
                    fullRoute
                );
                if (targets.empty()) {
                    bool routeEndsAtEnemy = fullRoute.empty() ||
                        (fullRoute.size() == 1 &&
                         AlmostSamePosition(
                             fullRoute.front(),
                             enemyPosition
                         ));
                    return {
                        routeEndsAtEnemy
                            ? EnemyPathStatus::AtGoal
                            : EnemyPathStatus::Unreachable,
                        {}
                    };
                }

                return { EnemyPathStatus::Ready, targets };
            }

            for (Tile direction : directions) {
                Tile neighbor = {
                    current.tile.x + direction.x,
                    current.tile.y + direction.y
                };

                if (closedSet.find(neighbor) != closedSet.end() ||
                    !CanMoveBetweenTiles(
                        levelManager,
                        enemy,
                        current.tile,
                        neighbor
                    )) {
                    continue;
                }

                bool isDiagonal = direction.x != 0 && direction.y != 0;
                float moveCost = isDiagonal ? DIAGONAL_COST : 1.0f;
                float tentativeGCost = current.gCost + moveCost;
                auto existingScore = gScore.find(neighbor);

                if (existingScore == gScore.end() ||
                    tentativeGCost < existingScore->second) {
                    cameFrom[neighbor] = current.tile;
                    gScore[neighbor] = tentativeGCost;

                    float fCost = tentativeGCost +
                        Heuristic(neighbor, playerTile);
                    openSet.push({ neighbor, tentativeGCost, fCost });
                }
            }
        }

        return { EnemyPathStatus::Unreachable, {} };
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
    PopReachedTargets(&enemy);

    if (enemy.HasTargetPosition()) {
        Vector2 targetPosition = enemy.FirstTargetPosition();
        if (IsBodyPathClear(
                &levelManager,
                &enemy,
                enemy.GetPosition(),
                targetPosition
            )) {
            return targetPosition;
        }

        enemy.ClearTargetPosition();
        enemy.SetPathStatus(EnemyPathStatus::Pending);
        return std::nullopt;
    }

    if (enemy.GetPathStatus() == EnemyPathStatus::Ready) {
        enemy.SetPathStatus(EnemyPathStatus::AtGoal);
    }
    return std::nullopt;
}

Vector2 EnemyPathManager::GetLocalAvoidanceDirection(
    LevelManager& levelManager,
    Enemy& enemy,
    Vector2 desiredDirection
) {
    Vector2 finalDirection = desiredDirection;
    Vector2 enemyPosition = enemy.GetPosition();
    const float separationRadius = 42.0f;
    const float separationWeight = 0.85f;

    for (GameObject* entity : levelManager.GetEntities()) {
        if (entity->GetObjectType() != GameObjectType::Enemy) {
            continue;
        }

        Enemy* otherEnemy = static_cast<Enemy*>(entity);
        if (otherEnemy == &enemy || otherEnemy->IsDead()) {
            continue;
        }

        Vector2 away = Vector2Subtract(
            enemyPosition,
            otherEnemy->GetPosition()
        );
        float distance = Vector2Length(away);
        if (distance > 0.001f && distance < separationRadius) {
            float strength =
                (separationRadius - distance) / separationRadius;
            finalDirection = Vector2Add(
                finalDirection,
                Vector2Scale(
                    Vector2Normalize(away),
                    strength * separationWeight
                )
            );
        }
    }

    if (Vector2Length(desiredDirection) > 0.001f) {
        const float probeDistance = 18.0f;
        Vector2 forwardProbe = Vector2Add(
            enemyPosition,
            Vector2Scale(desiredDirection, probeDistance)
        );

        if (!IsBodyClearAtWorldPosition(
                &levelManager,
                &enemy,
                forwardProbe
            )) {
            Vector2 left = { -desiredDirection.y, desiredDirection.x };
            Vector2 right = { desiredDirection.y, -desiredDirection.x };
            Vector2 leftProbe = Vector2Add(
                enemyPosition,
                Vector2Scale(left, probeDistance)
            );
            Vector2 rightProbe = Vector2Add(
                enemyPosition,
                Vector2Scale(right, probeDistance)
            );

            bool leftClear = IsBodyClearAtWorldPosition(
                &levelManager,
                &enemy,
                leftProbe
            );
            bool rightClear = IsBodyClearAtWorldPosition(
                &levelManager,
                &enemy,
                rightProbe
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
    }

    if (Vector2Length(finalDirection) > 0.001f) {
        return Vector2Normalize(finalDirection);
    }

    return desiredDirection;
}

void EnemyPathManager::Update(LevelManager& levelManager, float deltaTime) {
    if (enemies.empty()) return;
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

        Enemy* enemy = enemies[nextEnemyIndex];
        nextEnemyIndex++;

        if (!enemy || enemy->IsDead() || !enemy->GetTargetTeam()) {
            continue;
        }

        Paladin* player = enemy->GetTargetTeam()->GetActivePaladin();
        if (!player) {
            enemy->ClearTargetPosition();
            enemy->SetPathStatus(EnemyPathStatus::Unreachable);
            continue;
        }

        PathSearchResult result = FindPath(
            &levelManager,
            enemy,
            player->GetPosition()
        );
        enemy->SetTargetPositions(result.targets);
        enemy->SetPathStatus(result.status);
    }
}
