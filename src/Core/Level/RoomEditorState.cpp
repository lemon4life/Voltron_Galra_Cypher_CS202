#include "Core/Level/RoomEditorState.h"

#include "Core/Constants.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/GameManager.h"
#include "Entities/Props/Prop.h"
#include "UI/UIUtils.h"

#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float BASE_SCREEN_WIDTH = 1280.0f;
    constexpr float BASE_SCREEN_HEIGHT = 720.0f;
    constexpr float PREVIEW_OPACITY = 0.70f;
    constexpr float RECTANGLE_EPSILON = 0.01f;

    int StableTileHash(int x, int y) {
        unsigned int hash =
            static_cast<unsigned int>(x * 374761393 ^ y * 668265263);
        hash = (hash ^ (hash >> 13)) * 1274126177;
        return static_cast<int>(hash ^ (hash >> 16));
    }

    bool ContainsRectangle(Rectangle outer, Rectangle inner) {
        return inner.x >= outer.x - RECTANGLE_EPSILON &&
            inner.y >= outer.y - RECTANGLE_EPSILON &&
            inner.x + inner.width <=
                outer.x + outer.width + RECTANGLE_EPSILON &&
            inner.y + inner.height <=
                outer.y + outer.height + RECTANGLE_EPSILON;
    }

    bool RectanglesOverlap(Rectangle first, Rectangle second) {
        return first.x < second.x + second.width - RECTANGLE_EPSILON &&
            first.x + first.width > second.x + RECTANGLE_EPSILON &&
            first.y < second.y + second.height - RECTANGLE_EPSILON &&
            first.y + first.height > second.y + RECTANGLE_EPSILON;
    }
}

RoomEditorState::RoomEditorState()
    : camera({}),
      currentRoomSize(RoomSize::SMALL),
      currentBrush(BrushType::WALL),
      scrollOffset(0.0f),
      uiScale(1.0f),
      showGuide(false),
      statusMessage(""),
      statusTimer(0.0f),
      sidebarBounds({}),
      paletteBounds({}),
      lastScreenWidth(0),
      lastScreenHeight(0) {
    camera.zoom = 1.0f;
}

RoomEditorState::~RoomEditorState() = default;

void RoomEditorState::Initialize() {
    placedObjects.clear();
    scrollOffset = 0.0f;
    currentRoomPath.clear();

    AssetManager& assets = AssetManager::GetInstance();
    brushes.clear();
    brushes.push_back({
        static_cast<int>(BrushType::ERASER),
        "Eraser",
        {},
        "TOOLS",
        "Remove",
        {}
    });
    brushes.push_back({
        static_cast<int>(BrushType::WALL),
        "Wall",
        assets.LoadTexture2D(
            "Galra_Walls",
            "assets/tileset/Galra_Walls.png",
            true
        ),
        "TILES",
        "16x16",
        { 0.0f, 0.0f, 16.0f, 16.0f }
    });
    brushes.push_back({
        static_cast<int>(BrushType::BOX),
        "Box",
        assets.LoadTexture2D("box", "assets/Objects/box.png", true),
        "MAP OBJECTS",
        "16x32",
        { 0.0f, 0.0f, 16.0f, 32.0f }
    });
    brushes.push_back({
        static_cast<int>(BrushType::OBJECT_2),
        "Object",
        assets.LoadTexture2D(
            "object_2",
            "assets/Objects/object_2.png",
            true
        ),
        "MAP OBJECTS",
        "16x32",
        { 0.0f, 0.0f, 16.0f, 32.0f }
    });
    brushes.push_back({
        static_cast<int>(BrushType::POT_HP),
        "Pot HP",
        assets.LoadTexture2D("pot_hp", "assets/Objects/pot_hp.png", true),
        "MAP OBJECTS",
        "9x13",
        { 0.0f, 0.0f, 9.0f, 13.0f }
    });
    brushes.push_back({
        static_cast<int>(BrushType::POT_EX),
        "Pot EX",
        assets.LoadTexture2D("pot_ex", "assets/Objects/pot_ex.png", true),
        "MAP OBJECTS",
        "9x13",
        { 0.0f, 0.0f, 9.0f, 13.0f }
    });
    brushes.push_back({
        static_cast<int>(BrushType::POT_QUINT),
        "Pot Quint",
        assets.LoadTexture2D(
            "pot_quint",
            "assets/Objects/pot_quint.png",
            true
        ),
        "MAP OBJECTS",
        "9x13",
        { 0.0f, 0.0f, 9.0f, 13.0f }
    });
    brushes.push_back({
        static_cast<int>(BrushType::TALL_OBJECT),
        "Tall Object",
        assets.LoadTexture2D(
            "tall_object_1_8",
            "assets/Objects/tall_object_1_8.png",
            true
        ),
        "MAP OBJECTS",
        "32x64",
        { 0.0f, 0.0f, 32.0f, 64.0f }
    });

    currentBrush = BrushType::WALL;
    UpdateResponsiveLayout(true);
}

bool RoomEditorState::LoadRoom(const std::string& path) {
    RoomSize loadedSize = RoomSize::SMALL;
    std::vector<PlaceableObject> loadedObjects;
    if (!LevelIO::LoadRoomFromCSV(path, loadedSize, loadedObjects)) {
        statusMessage = "Failed to load room";
        statusTimer = 3.0f;
        return false;
    }

    currentRoomSize = loadedSize;
    placedObjects = std::move(loadedObjects);
    currentRoomPath = path;
    currentBrush = BrushType::WALL;
    scrollOffset = 0.0f;
    statusMessage = "Loaded: " + path;
    statusTimer = 2.0f;
    UpdateResponsiveLayout(true);
    return true;
}

Vector2 RoomEditorState::GetRoomDimensions(RoomSize size) const {
    switch (size) {
        case RoomSize::SMALL:
            return { 15.0f * GRID_SIZE, 15.0f * GRID_SIZE };
        case RoomSize::MEDIUM:
            return { 20.0f * GRID_SIZE, 20.0f * GRID_SIZE };
        case RoomSize::LARGE:
            return { 25.0f * GRID_SIZE, 25.0f * GRID_SIZE };
    }
    return { 15.0f * GRID_SIZE, 15.0f * GRID_SIZE };
}

void RoomEditorState::UpdateResponsiveLayout(bool centerCamera) {
    int screenWidth = std::max(1, GetScreenWidth());
    int screenHeight = std::max(1, GetScreenHeight());
    bool screenChanged =
        screenWidth != lastScreenWidth || screenHeight != lastScreenHeight;

    uiScale = std::clamp(
        std::min(
            static_cast<float>(screenWidth) / BASE_SCREEN_WIDTH,
            static_cast<float>(screenHeight) / BASE_SCREEN_HEIGHT
        ),
        0.65f,
        1.5f
    );

    float maximumSidebarWidth = screenWidth * 0.38f;
    float sidebarWidth = std::clamp(
        250.0f * uiScale,
        190.0f,
        std::min(360.0f, maximumSidebarWidth)
    );
    sidebarBounds = {
        0.0f,
        0.0f,
        sidebarWidth,
        static_cast<float>(screenHeight)
    };

    float paletteTop = 210.0f * uiScale;
    float bottomControlsHeight = 130.0f * uiScale;
    paletteBounds = {
        0.0f,
        paletteTop,
        sidebarBounds.width,
        std::max(
            0.0f,
            sidebarBounds.height - paletteTop - bottomControlsHeight
        )
    };

    lastScreenWidth = screenWidth;
    lastScreenHeight = screenHeight;
    if (centerCamera || screenChanged) {
        CenterCameraOnRoom();
    }
}

void RoomEditorState::CenterCameraOnRoom() {
    Vector2 roomSize = GetRoomDimensions(currentRoomSize);
    float canvasWidth = std::max(
        1.0f,
        static_cast<float>(GetScreenWidth()) - sidebarBounds.width
    );
    float canvasHeight = std::max(1.0f, static_cast<float>(GetScreenHeight()));
    float padding = 48.0f * uiScale;
    float availableWidth = std::max(1.0f, canvasWidth - padding * 2.0f);
    float availableHeight = std::max(1.0f, canvasHeight - padding * 2.0f);

    camera.target = { roomSize.x * 0.5f, roomSize.y * 0.5f };
    camera.offset = {
        sidebarBounds.width + canvasWidth * 0.5f,
        canvasHeight * 0.5f
    };
    camera.zoom = std::clamp(
        std::min(
            availableWidth / roomSize.x,
            availableHeight / roomSize.y
        ),
        0.25f,
        3.0f
    );
}

void RoomEditorState::Update(float deltaTime) {
    if (statusTimer > 0.0f) {
        statusTimer = std::max(0.0f, statusTimer - deltaTime);
    }

    UpdateResponsiveLayout(false);
    HandleInput();
}

void RoomEditorState::HandleInput() {
    if (showGuide) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
            IsKeyPressed(KEY_ESCAPE)) {
            showGuide = false;
        }
        return;
    }

    Vector2 mousePosition = GetMousePosition();
    float wheel = GetMouseWheelMove();

    if (CheckCollisionPointRec(mousePosition, sidebarBounds)) {
        if (wheel != 0.0f &&
            CheckCollisionPointRec(mousePosition, paletteBounds)) {
            int categoryCount = 0;
            std::string lastCategory;
            for (const EditorBrush& brush : brushes) {
                if (brush.category != lastCategory) {
                    lastCategory = brush.category;
                    categoryCount++;
                }
            }
            float contentHeight =
                brushes.size() * 50.0f * uiScale +
                categoryCount * 35.0f * uiScale;
            float minimumScroll = std::min(
                0.0f,
                paletteBounds.height - contentHeight - 10.0f * uiScale
            );
            scrollOffset = std::clamp(
                scrollOffset + wheel * 30.0f * uiScale,
                minimumScroll,
                0.0f
            );
        }
        return;
    }

    float panSpeed = 400.0f * GetFrameTime() / camera.zoom;
    if (IsKeyDown(KEY_W)) camera.target.y -= panSpeed;
    if (IsKeyDown(KEY_S)) camera.target.y += panSpeed;
    if (IsKeyDown(KEY_A)) camera.target.x -= panSpeed;
    if (IsKeyDown(KEY_D)) camera.target.x += panSpeed;

    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = Vector2Scale(GetMouseDelta(), -1.0f / camera.zoom);
        camera.target = Vector2Add(camera.target, delta);
    }

    if (wheel != 0.0f) {
        Vector2 worldBeforeZoom = GetScreenToWorld2D(mousePosition, camera);
        float scaleFactor = 1.0f + 0.25f * std::fabs(wheel);
        if (wheel < 0.0f) scaleFactor = 1.0f / scaleFactor;
        camera.zoom = std::clamp(
            camera.zoom * scaleFactor,
            0.25f,
            3.0f
        );
        Vector2 worldAfterZoom = GetScreenToWorld2D(mousePosition, camera);
        camera.target = Vector2Add(
            camera.target,
            Vector2Subtract(worldBeforeZoom, worldAfterZoom)
        );
    }

    Vector2 worldPosition = GetScreenToWorld2D(mousePosition, camera);
    int gridX = static_cast<int>(std::floor(worldPosition.x / GRID_SIZE));
    int gridY = static_cast<int>(std::floor(worldPosition.y / GRID_SIZE));

    if (currentBrush == BrushType::ERASER) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int hoveredIndex = GetHoveredObjectIndex(worldPosition);
            if (hoveredIndex >= 0) {
                placedObjects.erase(placedObjects.begin() + hoveredIndex);
            }
        }
        return;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
        currentBrush != BrushType::NONE) {
        PlaceObject(gridX, gridY);
    } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        EraseObject(gridX, gridY);
    }
}

const EditorBrush* RoomEditorState::GetBrush(int objectID) const {
    auto match = std::find_if(
        brushes.begin(),
        brushes.end(),
        [objectID](const EditorBrush& brush) {
            return brush.objectID == objectID;
        }
    );
    return match == brushes.end() ? nullptr : &(*match);
}

Rectangle RoomEditorState::GetPlacementSpriteBounds(
    const EditorBrush& brush,
    int gridX,
    int gridY
) const {
    Rectangle cell = {
        static_cast<float>(gridX * GRID_SIZE),
        static_cast<float>(gridY * GRID_SIZE),
        static_cast<float>(GRID_SIZE),
        static_cast<float>(GRID_SIZE)
    };
    Vector2 tileCenter = {
        cell.x + cell.width * 0.5f,
        cell.y + cell.height * 0.5f
    };

    if (brush.objectID == static_cast<int>(BrushType::WALL)) {
        return { cell.x, cell.y, cell.width, cell.height * 2.0f };
    }
    if (brush.objectID == static_cast<int>(BrushType::BOX)) {
        return Prop::GetDestructibleBoxSpriteBounds(tileCenter);
    }
    if (brush.objectID == static_cast<int>(BrushType::OBJECT_2)) {
        return Prop::GetMapObjectSpriteBounds(
            tileCenter,
            MapObjectId::Prop2
        );
    }
    if (brush.objectID == static_cast<int>(BrushType::TALL_OBJECT)) {
        return Prop::GetMapObjectSpriteBounds(
            tileCenter,
            MapObjectId::Prop1
        );
    }

    return {
        tileCenter.x - brush.sourceRect.width * 0.5f,
        tileCenter.y - brush.sourceRect.height * 0.5f,
        brush.sourceRect.width,
        brush.sourceRect.height
    };
}

Rectangle RoomEditorState::GetPlacementHitbox(
    const EditorBrush& brush,
    int gridX,
    int gridY
) const {
    Vector2 tileCenter = {
        gridX * GRID_SIZE + GRID_SIZE * 0.5f,
        gridY * GRID_SIZE + GRID_SIZE * 0.5f
    };
    if (brush.objectID == static_cast<int>(BrushType::BOX)) {
        return Prop::GetDestructibleBoxBoundingBox(tileCenter);
    }
    if (brush.objectID == static_cast<int>(BrushType::OBJECT_2)) {
        return Prop::GetMapObjectBoundingBox(tileCenter, MapObjectId::Prop2);
    }
    if (brush.objectID == static_cast<int>(BrushType::TALL_OBJECT)) {
        return Prop::GetMapObjectBoundingBox(tileCenter, MapObjectId::Prop1);
    }
    return GetPlacementCollisionBox(brush, gridX, gridY);
}

Rectangle RoomEditorState::GetPlacementCollisionBox(
    const EditorBrush& brush,
    int gridX,
    int gridY
) const {
    Rectangle cell = {
        static_cast<float>(gridX * GRID_SIZE),
        static_cast<float>(gridY * GRID_SIZE),
        static_cast<float>(GRID_SIZE),
        static_cast<float>(GRID_SIZE)
    };
    Vector2 tileCenter = {
        cell.x + cell.width * 0.5f,
        cell.y + cell.height * 0.5f
    };

    if (brush.objectID == static_cast<int>(BrushType::WALL)) {
        return cell;
    }
    if (brush.objectID == static_cast<int>(BrushType::BOX)) {
        return Prop::GetDestructibleBoxCollisionBox(tileCenter);
    }
    if (brush.objectID == static_cast<int>(BrushType::OBJECT_2)) {
        return Prop::GetMapObjectCollisionBox(tileCenter, MapObjectId::Prop2);
    }
    if (brush.objectID == static_cast<int>(BrushType::TALL_OBJECT)) {
        return Prop::GetMapObjectCollisionBox(tileCenter, MapObjectId::Prop1);
    }

    return GetPlacementSpriteBounds(brush, gridX, gridY);
}

bool RoomEditorState::CanPlaceObject(int gridX, int gridY) const {
    const EditorBrush* brush = GetBrush(static_cast<int>(currentBrush));
    if (!brush) return false;

    Vector2 roomSize = GetRoomDimensions(currentRoomSize);
    Rectangle interiorBounds = {
        static_cast<float>(GRID_SIZE),
        static_cast<float>(GRID_SIZE),
        roomSize.x - GRID_SIZE * 2.0f,
        roomSize.y - GRID_SIZE * 2.0f
    };
    Rectangle roomBounds = { 0.0f, 0.0f, roomSize.x, roomSize.y };
    Rectangle candidate = GetPlacementCollisionBox(*brush, gridX, gridY);
    Rectangle candidateSprite = GetPlacementSpriteBounds(
        *brush,
        gridX,
        gridY
    );
    if (!ContainsRectangle(interiorBounds, candidate) ||
        !ContainsRectangle(roomBounds, candidateSprite)) {
        return false;
    }

    for (const PlaceableObject& object : placedObjects) {
        const EditorBrush* placedBrush = GetBrush(object.objectID);
        if (!placedBrush) continue;
        Rectangle occupied = GetPlacementCollisionBox(
            *placedBrush,
            object.gridX,
            object.gridY
        );
        if (RectanglesOverlap(candidate, occupied)) return false;
    }
    return true;
}

int RoomEditorState::GetHoveredObjectIndex(Vector2 worldPosition) const {
    int hoveredIndex = -1;
    float hoveredDepth = -INFINITY;
    for (std::size_t index = 0; index < placedObjects.size(); ++index) {
        const PlaceableObject& object = placedObjects[index];
        const EditorBrush* brush = GetBrush(object.objectID);
        if (!brush) continue;

        Rectangle spriteBounds = GetPlacementSpriteBounds(
            *brush,
            object.gridX,
            object.gridY
        );
        if (!CheckCollisionPointRec(worldPosition, spriteBounds)) continue;

        Rectangle depthBounds = GetPlacementHitbox(
            *brush,
            object.gridX,
            object.gridY
        );
        float depth = depthBounds.y + depthBounds.height;
        if (hoveredIndex < 0 || depth >= hoveredDepth) {
            hoveredIndex = static_cast<int>(index);
            hoveredDepth = depth;
        }
    }
    return hoveredIndex;
}

void RoomEditorState::PlaceObject(int gridX, int gridY) {
    if (!CanPlaceObject(gridX, gridY)) return;
    placedObjects.push_back({
        static_cast<int>(currentBrush),
        gridX,
        gridY
    });
}

void RoomEditorState::EraseObject(int gridX, int gridY) {
    placedObjects.erase(
        std::remove_if(
            placedObjects.begin(),
            placedObjects.end(),
            [gridX, gridY](const PlaceableObject& object) {
                return object.gridX == gridX && object.gridY == gridY;
            }
        ),
        placedObjects.end()
    );
}

void RoomEditorState::Draw() {
    ClearBackground(DARKGRAY);

    Camera2D renderCamera = camera;
    renderCamera.target.x = std::floor(renderCamera.target.x);
    renderCamera.target.y = std::floor(renderCamera.target.y);
    renderCamera.offset.x = std::floor(renderCamera.offset.x);
    renderCamera.offset.y = std::floor(renderCamera.offset.y);

    BeginMode2D(renderCamera);
    DrawRoomShell();
    DrawPlacedObjects();
    DrawGridOverlay();
    DrawPlacementPreview();

    Vector2 roomSize = GetRoomDimensions(currentRoomSize);
    DrawRectangleLinesEx(
        { 0.0f, 0.0f, roomSize.x, roomSize.y },
        1.0f,
        RED
    );
    EndMode2D();

    DrawUI();
    if (showGuide) DrawGuidePanel();
}

void RoomEditorState::DrawRoomShell() {
    Vector2 bounds = GetRoomDimensions(currentRoomSize);
    Texture2D floorTexture =
        AssetManager::GetInstance().GetTexture("Galra_Floors");
    Texture2D wallTexture =
        AssetManager::GetInstance().GetTexture("Galra_Walls");

    Rectangle wallTopSource[2] = {
        { 0.1f, 0.1f, 15.8f, 15.8f },
        { 16.1f, 0.1f, 15.8f, 15.8f }
    };
    Rectangle wallFaceSource[2] = {
        { 0.1f, 16.1f, 15.8f, 15.8f },
        { 16.1f, 16.1f, 15.8f, 15.8f }
    };
    Rectangle floorSource[6];
    for (int index = 0; index < 6; ++index) {
        floorSource[index] = {
            index * 16.0f + 0.1f,
            0.1f,
            15.8f,
            15.8f
        };
    }

    for (int x = 0; x < static_cast<int>(bounds.x); x += GRID_SIZE) {
        for (int y = 0; y < static_cast<int>(bounds.y); y += GRID_SIZE) {
            bool perimeter =
                x == 0 || y == 0 ||
                x >= bounds.x - GRID_SIZE ||
                y >= bounds.y - GRID_SIZE;
            if (perimeter) continue;
            int variant = std::abs(StableTileHash(x, y)) % 6;
            DrawTexturePro(
                floorTexture,
                floorSource[variant],
                {
                    static_cast<float>(x),
                    static_cast<float>(y),
                    static_cast<float>(GRID_SIZE),
                    static_cast<float>(GRID_SIZE)
                },
                { 0.0f, 0.0f },
                0.0f,
                WHITE
            );
        }
    }

    for (int x = 0; x < static_cast<int>(bounds.x); x += GRID_SIZE) {
        for (int y = 0; y < static_cast<int>(bounds.y); y += GRID_SIZE) {
            bool perimeter =
                x == 0 || y == 0 ||
                x >= bounds.x - GRID_SIZE ||
                y >= bounds.y - GRID_SIZE;
            if (!perimeter) continue;

            int variant = std::abs(StableTileHash(x, y)) % 2;
            DrawTexturePro(
                wallTexture,
                wallFaceSource[variant],
                {
                    static_cast<float>(x),
                    static_cast<float>(y + GRID_SIZE),
                    static_cast<float>(GRID_SIZE),
                    static_cast<float>(GRID_SIZE)
                },
                { 0.0f, 0.0f },
                0.0f,
                WHITE
            );
            DrawTexturePro(
                wallTexture,
                wallTopSource[variant],
                {
                    static_cast<float>(x),
                    static_cast<float>(y),
                    static_cast<float>(GRID_SIZE),
                    static_cast<float>(GRID_SIZE)
                },
                { 0.0f, 0.0f },
                0.0f,
                WHITE
            );
        }
    }
}

void RoomEditorState::DrawGridOverlay() {
    Vector2 bounds = GetRoomDimensions(currentRoomSize);
    for (int x = 0; x <= static_cast<int>(bounds.x); x += GRID_SIZE) {
        DrawLine(
            x,
            0,
            x,
            static_cast<int>(bounds.y),
            ColorAlpha(WHITE, 0.15f)
        );
    }
    for (int y = 0; y <= static_cast<int>(bounds.y); y += GRID_SIZE) {
        DrawLine(
            0,
            y,
            static_cast<int>(bounds.x),
            y,
            ColorAlpha(WHITE, 0.15f)
        );
    }

    if (!Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES) return;
    for (int x = 0; x < static_cast<int>(bounds.x); x += GRID_SIZE) {
        for (int y = 0; y < static_cast<int>(bounds.y); y += GRID_SIZE) {
            bool perimeter =
                x == 0 || y == 0 ||
                x >= bounds.x - GRID_SIZE ||
                y >= bounds.y - GRID_SIZE;
            if (!perimeter) continue;
            DrawRectangleLinesEx(
                {
                    static_cast<float>(x),
                    static_cast<float>(y),
                    static_cast<float>(GRID_SIZE),
                    static_cast<float>(GRID_SIZE)
                },
                Constants::DEBUG_COLLISION_LINE_THICKNESS,
                GOLD
            );
        }
    }
}

void RoomEditorState::DrawBrush(
    const EditorBrush& brush,
    int gridX,
    int gridY,
    Color tint
) const {
    if (brush.texture.id == 0) return;

    if (brush.objectID == static_cast<int>(BrushType::WALL)) {
        int variant = std::abs(StableTileHash(gridX, gridY)) % 2;
        Rectangle topSource = {
            variant * 16.0f,
            0.0f,
            16.0f,
            16.0f
        };
        Rectangle faceSource = {
            variant * 16.0f,
            16.0f,
            16.0f,
            16.0f
        };
        float x = static_cast<float>(gridX * GRID_SIZE);
        float y = static_cast<float>(gridY * GRID_SIZE);
        DrawTexturePro(
            brush.texture,
            faceSource,
            { x, y + GRID_SIZE, static_cast<float>(GRID_SIZE), static_cast<float>(GRID_SIZE) },
            { 0.0f, 0.0f },
            0.0f,
            tint
        );
        DrawTexturePro(
            brush.texture,
            topSource,
            { x, y, static_cast<float>(GRID_SIZE), static_cast<float>(GRID_SIZE) },
            { 0.0f, 0.0f },
            0.0f,
            tint
        );
        return;
    }

    Rectangle destination = GetPlacementSpriteBounds(
        brush,
        gridX,
        gridY
    );
    DrawTexturePro(
        brush.texture,
        brush.sourceRect,
        destination,
        { 0.0f, 0.0f },
        0.0f,
        tint
    );
}

void RoomEditorState::DrawPlacementCollisionDebug(
    const EditorBrush& brush,
    int gridX,
    int gridY
) const {
    DrawRectangleLinesEx(
        GetPlacementHitbox(brush, gridX, gridY),
        Constants::DEBUG_COLLISION_LINE_THICKNESS * 2.0f,
        PURPLE
    );
    DrawRectangleLinesEx(
        GetPlacementCollisionBox(brush, gridX, gridY),
        Constants::DEBUG_COLLISION_LINE_THICKNESS,
        GOLD
    );
}

void RoomEditorState::DrawPlacedObjects() {
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    int hoveredIndex = currentBrush == BrushType::ERASER
        ? GetHoveredObjectIndex(mouseWorld)
        : -1;

    auto drawObject = [this, hoveredIndex](
        const PlaceableObject& object,
        int objectIndex
    ) {
        const EditorBrush* brush = GetBrush(object.objectID);
        if (!brush) {
            DrawRectangleRec(
                {
                    static_cast<float>(object.gridX * GRID_SIZE),
                    static_cast<float>(object.gridY * GRID_SIZE),
                    static_cast<float>(GRID_SIZE),
                    static_cast<float>(GRID_SIZE)
                },
                MAGENTA
            );
            return;
        }

        Color tint = objectIndex == hoveredIndex
            ? ColorAlpha(RED, PREVIEW_OPACITY)
            : WHITE;
        DrawBrush(*brush, object.gridX, object.gridY, tint);
        if (Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES) {
            DrawPlacementCollisionDebug(*brush, object.gridX, object.gridY);
        }
    };

    // Walls and objects share the editor depth queue. Higher map positions
    // render first, independent of placement order.
    std::vector<std::size_t> sortedObjects;
    sortedObjects.reserve(placedObjects.size());
    for (std::size_t index = 0; index < placedObjects.size(); ++index) {
        sortedObjects.push_back(index);
    }
    std::stable_sort(
        sortedObjects.begin(),
        sortedObjects.end(),
        [this](std::size_t firstIndex, std::size_t secondIndex) {
            const PlaceableObject& first = placedObjects[firstIndex];
            const PlaceableObject& second = placedObjects[secondIndex];
            const EditorBrush* firstBrush = GetBrush(first.objectID);
            const EditorBrush* secondBrush = GetBrush(second.objectID);
            if (!firstBrush || !secondBrush) {
                return first.gridY < second.gridY;
            }
            Rectangle firstBounds = GetPlacementHitbox(
                *firstBrush,
                first.gridX,
                first.gridY
            );
            Rectangle secondBounds = GetPlacementHitbox(
                *secondBrush,
                second.gridX,
                second.gridY
            );
            float firstDepth = firstBounds.y + firstBounds.height;
            float secondDepth = secondBounds.y + secondBounds.height;
            if (std::fabs(firstDepth - secondDepth) > RECTANGLE_EPSILON) {
                return firstDepth < secondDepth;
            }
            return first.gridX < second.gridX;
        }
    );
    for (std::size_t index : sortedObjects) {
        drawObject(placedObjects[index], static_cast<int>(index));
    }
}

void RoomEditorState::DrawPlacementPreview() {
    Vector2 mousePosition = GetMousePosition();
    if (CheckCollisionPointRec(mousePosition, sidebarBounds)) return;
    if (currentBrush == BrushType::ERASER) return;

    const EditorBrush* brush = GetBrush(static_cast<int>(currentBrush));
    if (!brush) return;

    Vector2 worldPosition = GetScreenToWorld2D(mousePosition, camera);
    int gridX = static_cast<int>(std::floor(worldPosition.x / GRID_SIZE));
    int gridY = static_cast<int>(std::floor(worldPosition.y / GRID_SIZE));
    bool valid = CanPlaceObject(gridX, gridY);
    Color previewTint = valid
        ? ColorAlpha(WHITE, PREVIEW_OPACITY)
        : ColorAlpha(RED, PREVIEW_OPACITY);

    DrawBrush(*brush, gridX, gridY, previewTint);
    DrawRectangleLinesEx(
        GetPlacementCollisionBox(*brush, gridX, gridY),
        1.0f,
        valid ? GREEN : RED
    );
    if (Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES) {
        DrawPlacementCollisionDebug(*brush, gridX, gridY);
    }
}

void RoomEditorState::DrawEditorText(
    const std::string& text,
    Vector2 position,
    float baseSize,
    Color color,
    bool centered
) const {
    Font font = AssetManager::GetInstance().GetCustomFont("PixeloidSans");
    float fontSize = std::max(10.0f, baseSize * uiScale);
    Vector2 textSize = MeasureTextEx(font, text.c_str(), fontSize, 1.0f);
    if (centered) {
        position.x -= textSize.x * 0.5f;
        position.y -= textSize.y * 0.5f;
    }
    DrawTextEx(font, text.c_str(), position, fontSize, 1.0f, color);
}

void RoomEditorState::DrawUI() {
    UIUtils::DrawPanel(sidebarBounds, Color{ 20, 20, 20, 240 });

    float padding = 20.0f * uiScale;
    float titleY = 20.0f * uiScale;
    DrawEditorText(
        "ROOM EDITOR",
        { sidebarBounds.width * 0.5f, titleY },
        28.0f,
        WHITE,
        true
    );

    Rectangle guideButton = {
        sidebarBounds.width - 40.0f * uiScale,
        10.0f * uiScale,
        30.0f * uiScale,
        30.0f * uiScale
    };
    Color guideColor = ORANGE;
    if (UIUtils::IsHovered(guideButton)) {
        guideColor = Fade(guideColor, 0.8f);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) showGuide = true;
    }
    DrawRectangleRec(guideButton, guideColor);
    DrawRectangleLinesEx(guideButton, std::max(1.0f, uiScale), BLACK);
    DrawEditorText(
        "?",
        {
            guideButton.x + guideButton.width * 0.5f,
            guideButton.y + guideButton.height * 0.5f
        },
        22.0f,
        WHITE,
        true
    );

    float buttonY = 70.0f * uiScale;
    float sizeButtonHeight = 30.0f * uiScale;
    float sizeButtonStep = 40.0f * uiScale;
    const char* sizeLabels[] = { "SMALL", "MEDIUM", "LARGE" };
    for (int index = 0; index < 3; ++index) {
        Rectangle button = {
            padding,
            buttonY,
            sidebarBounds.width - padding * 2.0f,
            sizeButtonHeight
        };
        bool selected = static_cast<int>(currentRoomSize) == index;
        Color buttonColor = selected ? ORANGE : LIGHTGRAY;
        if (UIUtils::IsHovered(button)) {
            buttonColor = Fade(buttonColor, 0.8f);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !selected) {
                currentRoomSize = static_cast<RoomSize>(index);
                placedObjects.clear();
                currentRoomPath.clear();
                scrollOffset = 0.0f;
                CenterCameraOnRoom();
            }
        }
        DrawRectangleRec(button, buttonColor);
        DrawRectangleLinesEx(button, std::max(1.0f, uiScale), BLACK);
        DrawEditorText(
            sizeLabels[index],
            {
                button.x + button.width * 0.5f,
                button.y + button.height * 0.5f
            },
            14.0f,
            BLACK,
            true
        );
        buttonY += sizeButtonStep;
    }

    float bottomButtonHeight = 40.0f * uiScale;
    float bottomGap = 10.0f * uiScale;
    float bottomMargin = 20.0f * uiScale;
    float exitY = sidebarBounds.height -
        bottomMargin - bottomButtonHeight * 2.0f - bottomGap;

    Rectangle exitButton = {
        padding,
        exitY,
        sidebarBounds.width - padding * 2.0f,
        bottomButtonHeight
    };
    Color exitColor = RED;
    if (UIUtils::IsHovered(exitButton)) {
        exitColor = Fade(exitColor, 0.8f);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            GameManager::GetInstance().SetState(GameState::MAIN_MENU);
        }
    }
    DrawRectangleRec(exitButton, exitColor);
    DrawRectangleLinesEx(exitButton, std::max(1.0f, uiScale), BLACK);
    DrawEditorText(
        "EXIT",
        {
            exitButton.x + exitButton.width * 0.5f,
            exitButton.y + exitButton.height * 0.5f
        },
        14.0f,
        WHITE,
        true
    );

    Rectangle saveButton = {
        padding,
        exitY + bottomButtonHeight + bottomGap,
        sidebarBounds.width - padding * 2.0f,
        bottomButtonHeight
    };
    Color saveColor = SKYBLUE;
    if (UIUtils::IsHovered(saveButton)) {
        saveColor = Fade(saveColor, 0.8f);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            std::string path = LevelIO::SaveRoomToCSV(
                currentRoomSize,
                placedObjects,
                currentRoomPath
            );
            statusMessage = path.empty()
                ? "Export Failed!"
                : "Export Successful: " + path;
            if (!path.empty()) currentRoomPath = path;
            statusTimer = 3.0f;
        }
    }
    DrawRectangleRec(saveButton, saveColor);
    DrawRectangleLinesEx(saveButton, std::max(1.0f, uiScale), BLACK);
    DrawEditorText(
        "SAVE CSV",
        {
            saveButton.x + saveButton.width * 0.5f,
            saveButton.y + saveButton.height * 0.5f
        },
        14.0f,
        BLACK,
        true
    );

    BeginScissorMode(
        static_cast<int>(paletteBounds.x),
        static_cast<int>(paletteBounds.y),
        static_cast<int>(paletteBounds.width),
        static_cast<int>(paletteBounds.height)
    );
    float paletteY = paletteBounds.y + scrollOffset;
    std::string category;
    for (const EditorBrush& brush : brushes) {
        if (brush.category != category) {
            category = brush.category;
            paletteY += 10.0f * uiScale;
            DrawEditorText(
                category,
                { sidebarBounds.width * 0.5f, paletteY },
                14.0f,
                ORANGE,
                true
            );
            paletteY += 25.0f * uiScale;
        }

        Rectangle button = {
            padding,
            paletteY,
            sidebarBounds.width - padding * 2.0f,
            45.0f * uiScale
        };
        bool selected =
            static_cast<int>(currentBrush) == brush.objectID;
        Color buttonColor = selected ? GREEN : LIGHTGRAY;
        bool inPalette = CheckCollisionPointRec(
            GetMousePosition(),
            paletteBounds
        );
        if (inPalette && UIUtils::IsHovered(button)) {
            buttonColor = Fade(buttonColor, 0.8f);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentBrush = static_cast<BrushType>(brush.objectID);
            }
        }
        DrawRectangleRec(button, buttonColor);
        DrawRectangleLinesEx(button, std::max(1.0f, uiScale), BLACK);

        float iconArea = 35.0f * uiScale;
        if (brush.texture.id != 0 &&
            brush.sourceRect.width > 0.0f &&
            brush.sourceRect.height > 0.0f) {
            float iconScale = std::min(
                iconArea / brush.sourceRect.width,
                iconArea / brush.sourceRect.height
            );
            Rectangle iconDestination = {
                button.x + 5.0f * uiScale +
                    (iconArea - brush.sourceRect.width * iconScale) * 0.5f,
                button.y + 5.0f * uiScale +
                    (iconArea - brush.sourceRect.height * iconScale) * 0.5f,
                brush.sourceRect.width * iconScale,
                brush.sourceRect.height * iconScale
            };
            DrawTexturePro(
                brush.texture,
                brush.sourceRect,
                iconDestination,
                { 0.0f, 0.0f },
                0.0f,
                WHITE
            );
        } else {
            DrawEditorText(
                "X",
                {
                    button.x + 5.0f * uiScale + iconArea * 0.5f,
                    button.y + 5.0f * uiScale + iconArea * 0.5f
                },
                20.0f,
                RED,
                true
            );
        }
        DrawEditorText(
            brush.name,
            {
                button.x + 45.0f * uiScale,
                button.y + 7.0f * uiScale
            },
            13.0f,
            BLACK
        );
        DrawEditorText(
            brush.sizeLabel,
            {
                button.x + 45.0f * uiScale,
                button.y + 25.0f * uiScale
            },
            11.0f,
            DARKGRAY
        );
        paletteY += 50.0f * uiScale;
    }
    EndScissorMode();

    if (statusTimer > 0.0f) {
        Color messageColor =
            statusMessage.find("Successful") != std::string::npos
            ? GREEN
            : RED;
        float canvasCenterX = sidebarBounds.width +
            (GetScreenWidth() - sidebarBounds.width) * 0.5f;
        DrawEditorText(
            statusMessage,
            { canvasCenterX, 36.0f * uiScale },
            22.0f,
            messageColor,
            true
        );
    }
}

void RoomEditorState::DrawGuidePanel() {
    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        ColorAlpha(BLACK, 0.8f)
    );

    float panelWidth = std::min(
        500.0f * uiScale,
        GetScreenWidth() - 40.0f * uiScale
    );
    float panelHeight = std::min(
        400.0f * uiScale,
        GetScreenHeight() - 40.0f * uiScale
    );
    Rectangle panel = {
        (GetScreenWidth() - panelWidth) * 0.5f,
        (GetScreenHeight() - panelHeight) * 0.5f,
        panelWidth,
        panelHeight
    };
    DrawRectangleRec(panel, DARKGRAY);
    DrawRectangleLinesEx(panel, 2.0f * uiScale, ORANGE);

    float centerX = panel.x + panel.width * 0.5f;
    float contentY = panel.y + 30.0f * uiScale;
    DrawEditorText(
        "ROOM EDITOR GUIDE",
        { centerX, contentY },
        26.0f,
        ORANGE,
        true
    );

    contentY += 55.0f * uiScale;
    float left = panel.x + 35.0f * uiScale;
    float lineStep = 38.0f * uiScale;
    DrawEditorText(
        "- Pan Camera: W/A/S/D or Middle Mouse",
        { left, contentY },
        14.0f,
        WHITE
    );
    contentY += lineStep;
    DrawEditorText(
        "- Zoom: Mouse Wheel outside sidebar",
        { left, contentY },
        14.0f,
        WHITE
    );
    contentY += lineStep;
    DrawEditorText(
        "- Place Wall/Object: Left Click",
        { left, contentY },
        14.0f,
        WHITE
    );
    contentY += lineStep;
    DrawEditorText(
        "- Erase Object: Right Click",
        { left, contentY },
        14.0f,
        WHITE
    );
    contentY += lineStep;
    DrawEditorText(
        "- Scroll Palette: Wheel over palette",
        { left, contentY },
        14.0f,
        WHITE
    );

    DrawEditorText(
        "Click anywhere to close",
        {
            centerX,
            panel.y + panel.height - 30.0f * uiScale
        },
        13.0f,
        GRAY,
        true
    );
}
