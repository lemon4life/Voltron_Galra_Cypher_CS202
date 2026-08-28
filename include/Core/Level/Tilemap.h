#pragma once
#include "Core/Level/RoomNode.h"
#include "Core/DepthRenderItem.h"
#include "raylib.h"
#include <vector>
#include <memory>
#include <string>

class RoomTemplate {
public:
    int width;
    int height;
    std::vector<std::vector<int>> layer0_tiles; // 0: floor, 1: wall, 2: pit
    std::vector<std::vector<int>> layer1_objects; // 10: spawn, 20: door, 30: enemy, 40: crate
    std::vector<std::vector<int>> layer2_props; // 0: empty, 1: box, 2: obj1, 3: obj2

    RoomTemplate() : width(0), height(0) {}
    RoomTemplate(int w, int h) : width(w), height(h) {
        layer0_tiles.resize(h, std::vector<int>(w, 0));
        layer1_objects.resize(h, std::vector<int>(w, 0));
        layer2_props.resize(h, std::vector<int>(w, 0));
    }

    // Generate absolute world coordinate bounding boxes for physics
    std::vector<Rectangle> GenerateWallColliders(Vector2 offsetWorldPos, float tileSize, float scale) const;
};

class LevelMap {
public:
    std::vector<std::vector<std::shared_ptr<RoomNode>>> grid;
    std::shared_ptr<RoomNode> spawnRoom;

    LevelMap() {}
    void Generate(
        int width,
        int height,
        int enemyRoomCount,
        int chestRoomCount,
        int enhanceRoomCount,
        bool bossFloor
    ); // macro layout generation
    std::shared_ptr<RoomTemplate> BakeLevel();
    std::vector<std::shared_ptr<RoomNode>> generatedNodes;
};

class TilemapRenderer {
public:
    static void DrawRoomBase(const RoomTemplate& room, Vector2 roomOffsetWorldPos, Texture2D floorTileset, Texture2D wallTileset, Texture2D prop1Texture, Texture2D prop2Texture, Texture2D boxTexture);
    static void GetRoomDepthRenderItems(const RoomTemplate& room, Vector2 roomOffsetWorldPos, Texture2D wallTileset, Texture2D prop1Texture, Texture2D prop2Texture, Texture2D boxTexture, std::vector<DepthRenderItem>& items);
};
