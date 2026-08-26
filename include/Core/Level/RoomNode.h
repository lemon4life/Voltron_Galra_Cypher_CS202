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
    EVENT,
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
          
        if (t == RoomType::BOSS) {
            roomSize = 25;
        } else if (t == RoomType::BATTLE) {
            // Small rooms (15) are strictly non-combat EVENT rooms. Medium rooms (20) are BATTLE rooms.
            if (GetRandomValue(0, 1) == 0) {
                roomSize = 15;
                type = RoomType::EVENT;
                isCleared = true;
                state = RoomState::CLEARED;
            } else {
                roomSize = 20;
            }
        } else if (t == RoomType::EVENT || t == RoomType::SPAWN) {
            roomSize = 15;
            isCleared = true;
            state = RoomState::CLEARED;
        } else {
            roomSize = 15;
        }
    }
          
    void CalculateWalkableGrid(class LevelManager* lm);
};
