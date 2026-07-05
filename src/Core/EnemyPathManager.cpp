#include "Core/EnemyPathManager.h"
#include "Core/GameManager.h"
#include "Core/LevelManager.h"
#include "Entities/Enemy.h"
#include "Entities/Player.h"

#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>

namespace {
    constexpr float TILE_SIZE = 32.0f;
    constexpr float DIAGONAL_COST = 1.41421356f;

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

    Rectangle GetBodyAtWorldPosition(Enemy* enemy, Vector2 worldPosition) {
        Rectangle currentBox = enemy->GetBoundingBox();
        Vector2 currentPosition = enemy->GetPosition();

        return {
            worldPosition.x - (currentPosition.x - currentBox.x),
            worldPosition.y - (currentPosition.y - currentBox.y),
            currentBox.width,
            currentBox.height
        };
    }

    bool IsBodyClearAtWorldPosition(LevelManager* levelManager, Enemy* enemy, Vector2 worldPosition) {
        if (!levelManager || !enemy) return false;

        Rectangle body = GetBodyAtWorldPosition(enemy, worldPosition);
        const float edgePadding = 0.001f;

        int leftTile = (int)std::floor((body.x + edgePadding) / TILE_SIZE);
        int rightTile = (int)std::floor((body.x + body.width - edgePadding) / TILE_SIZE);
        int topTile = (int)std::floor((body.y + edgePadding) / TILE_SIZE);
        int bottomTile = (int)std::floor((body.y + body.height - edgePadding) / TILE_SIZE);

        for (int y = topTile; y <= bottomTile; ++y) {
            for (int x = leftTile; x <= rightTile; ++x) {
                if (!levelManager->IsWalkableTile(x, y)) {
                    return false;
                }
            }
        }

        return true;
    }

    bool IsBodyClearAtTile(LevelManager* levelManager, Enemy* enemy, Tile tile) {
        if (!levelManager->IsWalkableTile(tile.x, tile.y)) return false;

        Vector2 worldPosition = levelManager->TileToWorld(tile.x, tile.y);
        return IsBodyClearAtWorldPosition(levelManager, enemy, worldPosition);
    }

    bool CanMoveBetweenTiles(LevelManager* levelManager, Enemy* enemy, Tile current, Tile neighbor) {
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

    bool HasReachedWaypoint(Enemy* enemy, Vector2 waypoint) {
        Rectangle body = enemy->GetBoundingBox();
        float reachDistance = std::max(4.0f, std::min(body.width, body.height) * 0.25f);

        Vector2 position = enemy->GetPosition();
        float dx = waypoint.x - position.x;
        float dy = waypoint.y - position.y;

        return (dx * dx + dy * dy) <= reachDistance * reachDistance;
    }

    std::vector<Tile> FindPath(LevelManager* levelManager, Enemy* enemy, Tile start, Tile goal) {
        if (!levelManager) return {};
        if (!enemy) return {};
        if (!IsBodyClearAtTile(levelManager, enemy, start)) return {};
        if (!IsBodyClearAtTile(levelManager, enemy, goal)) return {};

        std::priority_queue<AStarNode, std::vector<AStarNode>, CompareAStarNode> openSet;
        std::unordered_map<Tile, Tile, TileHash> cameFrom;
        std::unordered_map<Tile, float, TileHash> gScore;

        openSet.push({ start, 0.0f, Heuristic(start, goal) });
        gScore[start] = 0.0f;

        const Tile directions[8] = {
            { 1, 0 },
            { -1, 0 },
            { 0, 1 },
            { 0, -1 },
            { 1, 1 },
            { 1, -1 },
            { -1, 1 },
            { -1, -1 }
        };

        while (!openSet.empty()) {
            AStarNode current = openSet.top();
            openSet.pop();

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

void EnemyPathManager::RemoveEnemy(Enemy* enemy) {
    enemies.erase(
        std::remove(enemies.begin(), enemies.end(), enemy),
        enemies.end()
    );

    if (nextEnemyIndex >= enemies.size()) {
        nextEnemyIndex = 0;
    }
}

void EnemyPathManager::AddEnemy(Enemy* enemy) {
    if (std::find(enemies.begin(), enemies.end(), enemy) == enemies.end()) {
        enemies.push_back(enemy);
    }
}

void EnemyPathManager::Update(LevelManager* levelManager, float deltaTime) {
    // Currently deltaTime is not being used yet
    (void)deltaTime;

    if (!levelManager || enemies.empty()) return;

    int targetFPS = GameManager::GetInstance().GetTargetFPS();
    if (targetFPS <= 0) {
        targetFPS = 60;
    }

    int enemiesPerFrame = (int)std::ceil((float)enemies.size() / (float)targetFPS) * 2;
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

        if (!enemy || enemy->IsDead() || !enemy->GetTarget()) {
            continue;
        }

        if (enemy->HasTargetPosition()) {
            Vector2 currentTargetPosition = enemy->GetTargetPosition();
            if (!HasReachedWaypoint(enemy, currentTargetPosition) &&
                IsBodyClearAtWorldPosition(levelManager, enemy, currentTargetPosition)) {
                continue;
            }
        }

        Vector2 enemyTilePosition = levelManager->WorldToTile(enemy->GetPosition());
        Vector2 playerTilePosition = levelManager->WorldToTile(enemy->GetTarget()->GetPosition());

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
            enemy->SetTargetPosition(enemy->GetTarget()->GetPosition());
            continue;
        }

        Tile nextTile = path[1];
        Vector2 nextWorldPosition = levelManager->TileToWorld(nextTile.x, nextTile.y);
        enemy->SetTargetPosition(nextWorldPosition);
    }
}
