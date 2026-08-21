#pragma once

#include <memory>
#include <vector>
#include "raylib.h"

class DoorGate;


enum class RoomState {
    IDLE,
    LOCKED,
    CLEARED
};

enum class RoomType {
    SPAWN,
    BATTLE,
    CHEST,
    BOSS,
    EXIT
};

struct RoomNode {
    RoomType type;
    int gridX;
    int gridY;

    // Links to adjacent nodes
    std::shared_ptr<RoomNode> north;
    std::shared_ptr<RoomNode> south;
    std::shared_ptr<RoomNode> east;
    std::shared_ptr<RoomNode> west;

    bool isDiscovered;
    RoomState state;
    Rectangle triggerBounds; // in absolute world coords
    bool isCleared;
    
    Rectangle GetWorldBounds() const;

    std::vector<DoorGate*> doors;
    std::vector<Vector2> availableSpawnNodes;
    int roomSize;

    RoomNode(int x, int y, RoomType t = RoomType::BATTLE)
        : type(t), gridX(x), gridY(y),
          north(nullptr), south(nullptr), east(nullptr), west(nullptr),
          isDiscovered(false), isCleared(false), state(RoomState::IDLE) {
          
        if (t == RoomType::BOSS) roomSize = 25;
        else if (t == RoomType::BATTLE) {
            // Boss rooms are the unique largest room category.
            roomSize = (GetRandomValue(0, 1) == 0) ? 15 : 20;
        } else {
            roomSize = 15;
        }
    }
          
    void CalculateWalkableGrid(class LevelManager* lm);
};
