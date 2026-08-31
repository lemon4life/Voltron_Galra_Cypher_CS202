#pragma once

#include "raylib.h"

#include <cstdint>
#include <vector>

// Stable checkpoint data only. Battles, enemies, projectiles, effects, paths,
// and raw pointers are intentionally excluded so a loaded room begins cleanly.
struct SavedRoomState {
    int gridX = 0;
    int gridY = 0;
    int type = 0;
    int roomSize = 0;
    bool discovered = false;
    bool cleared = false;
    int state = 0;
    Rectangle triggerBounds = {};
};

struct SavedMapObjectState {
    int type = -1;
    Vector2 position = {};
    int row = -1;
    int column = -1;
    bool door = false;
    int doorState = 0;
    bool projectileBarrier = false;
};

struct SavedLevelState {
    int width = 0;
    int height = 0;
    int graphWidth = 0;
    int graphHeight = 0;
    int spawnRoomX = -1;
    int spawnRoomY = -1;
    Vector2 roomOffset = {};
    std::vector<int> floorTiles;
    std::vector<int> objectTiles;
    std::vector<int> propTiles;
    std::vector<SavedRoomState> rooms;
    std::vector<SavedMapObjectState> mapObjects;
};

struct SavedPaladinState {
    int id = 0;
    Vector2 position = {};
    Vector2 aimTarget = {};
    int health = 0;
    int maxHealth = 0;
    float ghostHealth = 0.0f;
    float displayedHealth = 0.0f;
    float exEnergy = 0.0f;
    float displayedExEnergy = 0.0f;
    float attackCooldown = 0.0f;
    float dashCooldown = 0.0f;
    float ultimateCooldown = 0.0f;
    float activeSkillDuration = 0.0f;
    float activeSkillTimer = 0.0f;
    int paladinLevel = 1;
    bool facingLeft = false;
    bool skillActive = false;
};

struct SavedTeamState {
    std::vector<SavedPaladinState> roster;
    std::vector<int> selectedSlots;
    int activeIndex = 0;
    int sharedArmor = 0;
    int maxSharedArmor = 0;
    float quintessence = 0.0f;
    int coins = 0;
};

struct SavedDynamicObject {
    int type = -1;
    Vector2 position = {};
};

struct MissionSaveData {
    static constexpr std::uint32_t VERSION = 1;

    int floor = 1;
    bool talkedToShiro = false;
    bool autoAim = false;
    SavedLevelState level;
    SavedTeamState team;
    std::vector<SavedDynamicObject> utilityObjects;
};
