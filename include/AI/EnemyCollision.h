#pragma once

#include "raylib.h"

class Enemy;
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
    /// Checks player attack overlap.
    static bool CheckPlayerAttackOverlap(
        const Enemy& enemy,
        const Paladin& player
    );

    /// Checks parry.
    static bool CheckParry(
        const Enemy& enemy,
        const Paladin& player
    );

    /// Moves against walls.
    static EnemyMoveResult MoveAgainstWalls(
        Enemy& enemy,
        Vector2 displacement,
        const IEnemyPathAccess& pathAccess,
        EnemyWallResponse response
    );
};
