#pragma once

#include <memory>
#include "raylib.h"


enum class RoomState {
    IDLE,
    LOCKED,
    CLEARED
};

enum class RoomType {
    SPAWN,
    BATTLE,
    CHEST,
    BOSS
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

    RoomNode(int x, int y, RoomType t = RoomType::BATTLE)
        : type(t), gridX(x), gridY(y),
          north(nullptr), south(nullptr), east(nullptr), west(nullptr),
          isDiscovered(false), isCleared(false), state(RoomState::IDLE) {}
};
