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
    
    /// Handles input.
    void HandleInput();
    /// Updates responsive layout.
    void UpdateResponsiveLayout(bool centerCamera);
    /// Centers the editor camera on the active room after its size or viewport changes.
    void CenterCameraOnRoom();
    /// Renders room shell.
    void DrawRoomShell();
    /// Renders grid overlay.
    void DrawGridOverlay();
    /// Renders ui.
    void DrawUI();
    /// Renders guide panel.
    void DrawGuidePanel();
    /// Renders placed objects.
    void DrawPlacedObjects();
    /// Renders placement preview.
    void DrawPlacementPreview();
    /// Renders brush.
    void DrawBrush(
        const EditorBrush& brush,
        int gridX,
        int gridY,
        Color tint
    ) const;
    /// Renders placement collision debug.
    void DrawPlacementCollisionDebug(
        const EditorBrush& brush,
        int gridX,
        int gridY
    ) const;
    /// Renders editor text.
    void DrawEditorText(
        const std::string& text,
        Vector2 position,
        float baseSize,
        Color color,
        bool centered = false
    ) const;
    
    /// Returns the current room dimensions.
    Vector2 GetRoomDimensions(RoomSize size) const;
    /// Returns the current brush.
    const EditorBrush* GetBrush(int objectID) const;
    /// Returns the current placement sprite bounds.
    Rectangle GetPlacementSpriteBounds(
        const EditorBrush& brush,
        int gridX,
        int gridY
    ) const;
    /// Returns the current placement hitbox.
    Rectangle GetPlacementHitbox(
        const EditorBrush& brush,
        int gridX,
        int gridY
    ) const;
    /// Returns the current placement collision box.
    Rectangle GetPlacementCollisionBox(
        const EditorBrush& brush,
        int gridX,
        int gridY
    ) const;
    /// Reports whether this component can perform place object.
    bool CanPlaceObject(int gridX, int gridY) const;
    /// Returns the current hovered object index.
    int GetHoveredObjectIndex(Vector2 worldPosition) const;
    /// Adds the selected wall or prop at the validated editor position.
    void PlaceObject(int gridX, int gridY);
    /// Removes the wall or prop currently selected by the eraser.
    void EraseObject(int gridX, int gridY);
    
public:
    /// Creates a RoomEditorState instance from the supplied configuration.
    RoomEditorState();
    /// Releases resources owned by this RoomEditorState instance.
    ~RoomEditorState();
    
    /// Initializes the resources and collaborators required before this component can run.
    void Initialize();
    /// Loads room.
    bool LoadRoom(const std::string& path);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime);
    /// Renders this component using its current state and visual resources.
    void Draw();
};
