#pragma once

#include <cstddef>
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
    EVENT,
    BOSS,
    EXIT
};

struct RoomNode {
    inline static std::size_t liveCount = 0;
    RoomType type;
    int gridX;
    int gridY;

    // Links to adjacent nodes
    RoomNode* north;
    RoomNode* south;
    RoomNode* east;
    RoomNode* west;

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
        ++liveCount;
          
        if (t == RoomType::BOSS) {
            roomSize = 25;
        } else if (t == RoomType::BATTLE) {
            roomSize = 20;
        } else if (t == RoomType::EVENT || t == RoomType::CHEST || t == RoomType::SPAWN || t == RoomType::EXIT) {
            roomSize = 15;
            isCleared = true;
            state = RoomState::CLEARED;
        } else {
            roomSize = 15;
        }
    }

    ~RoomNode() { --liveCount; }

    RoomNode(const RoomNode&) = delete;
    RoomNode& operator=(const RoomNode&) = delete;
          
    void CalculateWalkableGrid(class LevelManager* lm);
};
