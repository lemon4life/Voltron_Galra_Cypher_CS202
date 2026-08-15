#pragma once

#include "raylib.h"
#include "Core/Level/LevelIO.h"
#include <vector>
#include <string>

enum class BrushType {
    NONE = 0,
    WALL = 1,
    FLOOR = 2,
    SPAWNER = 3,
    POWERUP = 4,
    BOX = 5,
    OBJECT_2 = 6,
    POT_EX = 7,
    POT_HP = 8,
    POT_QUINT = 9,
    TALL_OBJECT = 10
};

struct EditorBrush {
    int objectID;
    std::string name;
    Texture2D texture;
    std::string category;
    std::string sizeLabel;
    Rectangle sourceRect;
};

class RoomEditorState {
private:
    Camera2D camera;
    RoomSize currentRoomSize;
    BrushType currentBrush;
    std::vector<PlaceableObject> placedObjects;
    std::vector<EditorBrush> brushes;
    
    const int GRID_SIZE = 32;
    float scrollOffset;
    bool showGuide;
    std::string statusMessage;
    float statusTimer;
    
    // UI bounds
    Rectangle sidebarBounds;
    
    void HandleInput();
    void DrawRoomShell();
    void DrawGridOverlay();
    void DrawUI();
    void DrawGuidePanel();
    void DrawPlacedObjects();
    
    Vector2 GetRoomDimensions(RoomSize size) const;
    void PlaceObject(int gridX, int gridY);
    void EraseObject(int gridX, int gridY);
    
public:
    RoomEditorState();
    ~RoomEditorState();
    
    void Initialize();
    void Update(float deltaTime);
    void Draw();
};
