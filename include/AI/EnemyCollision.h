#pragma once

#include "raylib.h"

#include <vector>

class Enemy;
class GameObject;
class IEnemyPathAccess;
class Paladin;

enum class EnemyWallResponse {
    Slide,
    Stop
};

struct EnemyMoveResult {
    Vector2 finalPosition = { 0.0f, 0.0f };
    bool blockedX = false;
    bool blockedY = false;
    bool hitWall = false;
};

struct EnemyCollision {
    static bool CheckPlayerCollision(
        const Enemy& enemy,
        const Paladin& player
    );

    static bool CheckEnemyCollision(
        const Enemy& enemy,
        const Enemy& other
    );

    static bool CheckAnyEnemyCollision(
        const Enemy& enemy,
        const std::vector<GameObject*>& entities
    );

    static bool CheckParry(
        const Enemy& enemy,
        const Paladin& player
    );

    static EnemyMoveResult MoveAgainstWalls(
        Enemy& enemy,
        Vector2 displacement,
        const IEnemyPathAccess& pathAccess,
        EnemyWallResponse response
    );
};
