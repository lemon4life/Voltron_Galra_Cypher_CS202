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
    
    std::vector<DoorGate*> doors;
    std::vector<Vector2> availableSpawnNodes;

    RoomNode(int x, int y, RoomType t = RoomType::BATTLE)
        : type(t), gridX(x), gridY(y),
          north(nullptr), south(nullptr), east(nullptr), west(nullptr),
          isDiscovered(false), isCleared(false), state(RoomState::IDLE) {}
          
    void CalculateWalkableGrid(class LevelManager* lm);
};
