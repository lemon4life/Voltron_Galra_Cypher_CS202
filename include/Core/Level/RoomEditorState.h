#pragma once

#include "raylib.h"
#include "Core/Level/LevelIO.h"
#include <vector>
#include <string>

enum class BrushType {
    ERASER = -2,
    NONE = 0,
    WALL = 1,
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
    
    const int GRID_SIZE = 16;
    float scrollOffset;
    float uiScale;
    bool showGuide;
    bool waitForPlacementRelease;
    std::string statusMessage;
    std::string currentRoomPath;
    float statusTimer;
    
    // UI bounds
    Rectangle sidebarBounds;
    Rectangle paletteBounds;
    int lastScreenWidth;
    int lastScreenHeight;
    
    void HandleInput();
    void UpdateResponsiveLayout(bool centerCamera);
    void CenterCameraOnRoom();
    void DrawRoomShell();
    void DrawGridOverlay();
    void DrawUI();
    void DrawGuidePanel();
    void DrawPlacedObjects();
    void DrawPlacementPreview();
    void DrawBrush(
        const EditorBrush& brush,
        int gridX,
        int gridY,
        Color tint
    ) const;
    void DrawPlacementCollisionDebug(
        const EditorBrush& brush,
        int gridX,
        int gridY
    ) const;
    void DrawEditorText(
        const std::string& text,
        Vector2 position,
        float baseSize,
        Color color,
        bool centered = false
    ) const;
    
    Vector2 GetRoomDimensions(RoomSize size) const;
    const EditorBrush* GetBrush(int objectID) const;
    Rectangle GetPlacementSpriteBounds(
        const EditorBrush& brush,
        int gridX,
        int gridY
    ) const;
    Rectangle GetPlacementHitbox(
        const EditorBrush& brush,
        int gridX,
        int gridY
    ) const;
    Rectangle GetPlacementCollisionBox(
        const EditorBrush& brush,
        int gridX,
        int gridY
    ) const;
    bool CanPlaceObject(int gridX, int gridY) const;
    int GetHoveredObjectIndex(Vector2 worldPosition) const;
    void PlaceObject(int gridX, int gridY);
    void EraseObject(int gridX, int gridY);
    
public:
    RoomEditorState();
    ~RoomEditorState();
    
    void Initialize();
    bool LoadRoom(const std::string& path);
    void Update(float deltaTime);
    void Draw();
};
