#include "Core/Level/RoomEditorState.h"
#include "Core/Manager/GameManager.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"
#include "raymath.h"
#include <algorithm>
#include <iostream>

RoomEditorState::RoomEditorState() {
    currentRoomSize = RoomSize::SMALL;
    currentBrush = BrushType::WALL;
    camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    sidebarBounds = { 0, 0, 250, (float)GetScreenHeight() };
    scrollOffset = 0.0f;
    showGuide = false;
    statusMessage = "";
    statusTimer = 0.0f;
}

RoomEditorState::~RoomEditorState() {
}

void RoomEditorState::Initialize() {
    placedObjects.clear();
    camera.target = { 0.0f, 0.0f };
    
    brushes.clear();
    brushes.push_back({ static_cast<int>(BrushType::WALL), "Wall", AssetManager::GetInstance().LoadTexture2D("Galra_Walls", "assets/tileset/Galra_Walls.png", true), "TILES", "32x32", {0, 0, 16, 16} });
    brushes.push_back({ static_cast<int>(BrushType::FLOOR), "Floor", AssetManager::GetInstance().LoadTexture2D("Galra_Floors", "assets/tileset/Galra_Floors.png", true), "TILES", "32x32", {0, 0, 16, 16} });
    
    brushes.push_back({ static_cast<int>(BrushType::BOX), "Box", AssetManager::GetInstance().LoadTexture2D("box", "assets/Objects/box.png", true), "OBJECTS", "16x32", {0, 0, 16, 32} });
    brushes.push_back({ static_cast<int>(BrushType::OBJECT_2), "Object 2", AssetManager::GetInstance().LoadTexture2D("object_2", "assets/Objects/object_2.png", true), "OBJECTS", "16x32", {0, 0, 16, 32} });
    brushes.push_back({ static_cast<int>(BrushType::POT_HP), "Pot HP", AssetManager::GetInstance().LoadTexture2D("pot_hp", "assets/Objects/pot_hp.png", true), "OBJECTS", "16x16", {0, 0, 12, 16} });
    brushes.push_back({ static_cast<int>(BrushType::POT_EX), "Pot EX", AssetManager::GetInstance().LoadTexture2D("pot_ex", "assets/Objects/pot_ex.png", true), "OBJECTS", "16x16", {0, 0, 12, 16} });
    brushes.push_back({ static_cast<int>(BrushType::POT_QUINT), "Pot Quint", AssetManager::GetInstance().LoadTexture2D("pot_quint", "assets/Objects/pot_quint.png", true), "OBJECTS", "16x16", {0, 0, 12, 16} });
    brushes.push_back({ static_cast<int>(BrushType::TALL_OBJECT), "Tall Object", AssetManager::GetInstance().LoadTexture2D("tall_object_1_8", "assets/Objects/tall_object_1_8.png", true), "OBJECTS", "32x64", {0, 0, 32, 64} });
    
}

Vector2 RoomEditorState::GetRoomDimensions(RoomSize size) const {
    switch (size) {
        case RoomSize::SMALL: return { 15.0f * GRID_SIZE, 15.0f * GRID_SIZE };
        case RoomSize::MEDIUM: return { 20.0f * GRID_SIZE, 20.0f * GRID_SIZE };
        case RoomSize::LARGE: return { 25.0f * GRID_SIZE, 25.0f * GRID_SIZE };
    }
    return { 15.0f * GRID_SIZE, 15.0f * GRID_SIZE };
}

void RoomEditorState::Update(float deltaTime) {
    if (statusTimer > 0.0f) {
        statusTimer -= deltaTime;
    }
    
    // Update sidebar bounds in case of resize
    sidebarBounds = { 0, 0, 250, (float)GetScreenHeight() };
    
    HandleInput();
}

void RoomEditorState::HandleInput() {
    if (showGuide) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ESCAPE)) {
            showGuide = false;
        }
        return; // Block other inputs while guide is open
    }

    Vector2 mousePos = GetMousePosition();
    float wheel = GetMouseWheelMove();

    if (CheckCollisionPointRec(mousePos, sidebarBounds)) {
        if (wheel != 0) {
            scrollOffset += wheel * 30.0f;
            if (scrollOffset > 0) scrollOffset = 0;
            // rough clamp for bottom
            float maxScroll = -((float)brushes.size() * 50.0f);
            if (scrollOffset < maxScroll) scrollOffset = maxScroll;
        }
    } else {
        // Camera Panning with Middle Mouse or WASD
        float panSpeed = 400.0f * GetFrameTime() / camera.zoom;
        
        if (IsKeyDown(KEY_W)) camera.target.y -= panSpeed;
        if (IsKeyDown(KEY_S)) camera.target.y += panSpeed;
        if (IsKeyDown(KEY_A)) camera.target.x -= panSpeed;
        if (IsKeyDown(KEY_D)) camera.target.x += panSpeed;
        
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / camera.zoom);
            camera.target = Vector2Add(camera.target, delta);
        }
        
        // Zooming
        if (wheel != 0) {
            Vector2 mouseWorldPos = GetScreenToWorld2D(mousePos, camera);
            camera.offset = GetMousePosition();
            camera.target = mouseWorldPos;
            
            float scaleFactor = 1.0f + (0.25f * fabsf(wheel));
            if (wheel < 0) scaleFactor = 1.0f / scaleFactor;
            camera.zoom = Clamp(camera.zoom * scaleFactor, 0.25f, 3.0f);
        }
        
        // Placement / Eraser
        Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
        int gridX = (int)floor(worldPos.x / GRID_SIZE);
        int gridY = (int)floor(worldPos.y / GRID_SIZE);
        
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && currentBrush != BrushType::NONE) {
            PlaceObject(gridX, gridY);
        } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            EraseObject(gridX, gridY);
        }
    }
}

void RoomEditorState::PlaceObject(int gridX, int gridY) {
    EraseObject(gridX, gridY); // Ensure no duplicate on same cell
    PlaceableObject obj;
    obj.objectID = static_cast<int>(currentBrush);
    obj.gridX = gridX;
    obj.gridY = gridY;
    placedObjects.push_back(obj);
}

void RoomEditorState::EraseObject(int gridX, int gridY) {
    placedObjects.erase(std::remove_if(placedObjects.begin(), placedObjects.end(),
        [gridX, gridY](const PlaceableObject& obj) {
            return obj.gridX == gridX && obj.gridY == gridY;
        }), placedObjects.end());
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
    
    // Draw boundary box
    Vector2 bounds = GetRoomDimensions(currentRoomSize);
    DrawRectangleLinesEx({0, 0, bounds.x, bounds.y}, 2.0f, RED);
    
    // Draw hover cursor
    Vector2 mousePos = GetMousePosition();
    if (!CheckCollisionPointRec(mousePos, sidebarBounds)) {
        Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
        int gridX = (int)floor(worldPos.x / GRID_SIZE);
        int gridY = (int)floor(worldPos.y / GRID_SIZE);
        DrawRectangleLines(gridX * GRID_SIZE, gridY * GRID_SIZE, GRID_SIZE, GRID_SIZE, YELLOW);
    }
    
    EndMode2D();
    
    DrawUI();
    
    if (showGuide) {
        DrawGuidePanel();
    }
}

void RoomEditorState::DrawRoomShell() {
    Vector2 bounds = GetRoomDimensions(currentRoomSize);
    Texture2D floorTex = AssetManager::GetInstance().GetTexture("Galra_Floors");
    Texture2D wallTex = AssetManager::GetInstance().GetTexture("Galra_Walls");
    
    Rectangle wallTopSrc[2] = { {0.1f, 0.1f, 15.8f, 15.8f}, {16.1f, 0.1f, 15.8f, 15.8f} };
    Rectangle wallFrontFaceSrc[2] = { {0.1f, 16.1f, 15.8f, 15.8f}, {16.1f, 16.1f, 15.8f, 15.8f} };
    Rectangle floorSrc[6];
    for(int i = 0; i < 6; ++i) {
        floorSrc[i] = { (float)(i * 16) + 0.1f, 0.1f, 15.8f, 15.8f };
    }
    
    auto hash = [](int x, int y) -> int {
        unsigned int h = (unsigned int)(x * 374761393 ^ y * 668265263);
        h = (h ^ (h >> 13)) * 1274126177;
        return h ^ (h >> 16);
    };
    
    // Draw floors (base)
    for (int x = 0; x < bounds.x; x += GRID_SIZE) {
        for (int y = 0; y < bounds.y; y += GRID_SIZE) {
            if (x == 0 || x >= bounds.x - GRID_SIZE || y == 0 || y >= bounds.y - GRID_SIZE) continue;
            int variant = std::abs(hash(x, y)) % 6;
            Rectangle dest = { (float)x, (float)y, (float)GRID_SIZE, (float)GRID_SIZE };
            DrawTexturePro(floorTex, floorSrc[variant], dest, {0,0}, 0.0f, WHITE);
        }
    }
    
    // Draw walls (perimeter)
    for (int x = 0; x < bounds.x; x += GRID_SIZE) {
        for (int y = 0; y < bounds.y; y += GRID_SIZE) {
            if (x == 0 || x >= bounds.x - GRID_SIZE || y == 0 || y >= bounds.y - GRID_SIZE) {
                int variant = std::abs(hash(x, y)) % 2;
                
                // If it's the bottom edge (not counting corners if we want them to fall off, but we'll just check if below is floor)
                // Since this is perimeter, below perimeter is void. But we draw front face if there's no wall below.
                bool isFloorBelow = false;
                if (y + GRID_SIZE < bounds.y) {
                    if (x > 0 && x < bounds.x - GRID_SIZE) isFloorBelow = true;
                } else {
                    isFloorBelow = true; // Bottom most edge will show front face
                }
                
                if (isFloorBelow) {
                    Rectangle destFace = { (float)x, (float)y + GRID_SIZE, (float)GRID_SIZE, (float)GRID_SIZE };
                    DrawTexturePro(wallTex, wallFrontFaceSrc[variant], destFace, {0,0}, 0.0f, WHITE);
                }
                
                Rectangle destTop = { (float)x, (float)y, (float)GRID_SIZE, (float)GRID_SIZE };
                DrawTexturePro(wallTex, wallTopSrc[variant], destTop, {0,0}, 0.0f, WHITE);
            }
        }
    }
}

void RoomEditorState::DrawGridOverlay() {
    Vector2 bounds = GetRoomDimensions(currentRoomSize);
    for (int x = 0; x <= bounds.x; x += GRID_SIZE) {
        DrawLine(x, 0, x, bounds.y, ColorAlpha(WHITE, 0.15f));
    }
    for (int y = 0; y <= bounds.y; y += GRID_SIZE) {
        DrawLine(0, y, bounds.x, y, ColorAlpha(WHITE, 0.15f));
    }
}

void RoomEditorState::DrawPlacedObjects() {
    auto hash = [](int x, int y) -> int {
        unsigned int h = (unsigned int)(x * 374761393 ^ y * 668265263);
        h = (h ^ (h >> 13)) * 1274126177;
        return h ^ (h >> 16);
    };
    
    Rectangle wallTopSrc[2] = { {0, 0, 16, 16}, {16, 0, 16, 16} };
    Rectangle wallFrontFaceSrc[2] = { {0, 16, 16, 16}, {16, 16, 16, 16} };
    Rectangle floorSrc[6];
    for(int i = 0; i < 6; ++i) {
        floorSrc[i] = { (float)(i * 16), 0.0f, 16.0f, 16.0f };
    }

    for (const auto& obj : placedObjects) {
        const EditorBrush* activeBrush = nullptr;
        for (const auto& brush : brushes) {
            if (brush.objectID == obj.objectID) {
                activeBrush = &brush;
                break;
            }
        }
        
        if (activeBrush && activeBrush->texture.id != 0) {
            float destX = obj.gridX * GRID_SIZE;
            float destY = obj.gridY * GRID_SIZE;
            
            if (obj.objectID == static_cast<int>(BrushType::WALL)) {
                int variant = std::abs(hash(obj.gridX, obj.gridY)) % 2;
                
                // Simple logic: if placed wall, draw front face at y+1
                Rectangle destFace = { destX, destY + GRID_SIZE, (float)GRID_SIZE, (float)GRID_SIZE };
                DrawTexturePro(activeBrush->texture, wallFrontFaceSrc[variant], destFace, {0,0}, 0.0f, WHITE);
                
                Rectangle destTop = { destX, destY, (float)GRID_SIZE, (float)GRID_SIZE };
                DrawTexturePro(activeBrush->texture, wallTopSrc[variant], destTop, {0,0}, 0.0f, WHITE);
            } else if (obj.objectID == static_cast<int>(BrushType::FLOOR)) {
                int variant = std::abs(hash(obj.gridX, obj.gridY)) % 6;
                Rectangle dest = { destX, destY, (float)GRID_SIZE, (float)GRID_SIZE };
                DrawTexturePro(activeBrush->texture, floorSrc[variant], dest, {0,0}, 0.0f, WHITE);
            } else {
                // Objects, use their source rect but scale to GRID_SIZE based on their grid width/height
                // The object's anchor is the tile it's placed on (bottom-left)
                float gridWidth = activeBrush->sourceRect.width / 16.0f;
                float gridHeight = activeBrush->sourceRect.height / 16.0f;
                
                // Center smaller objects
                float offsetX = 0.0f;
                float offsetY = 0.0f;
                if (activeBrush->sourceRect.width < 16.0f) {
                    offsetX = (16.0f - activeBrush->sourceRect.width) / 16.0f * (GRID_SIZE / 2.0f);
                }
                
                Rectangle dest = { 
                    destX + offsetX, 
                    destY - (gridHeight - 1.0f) * GRID_SIZE + offsetY, 
                    gridWidth * GRID_SIZE, 
                    gridHeight * GRID_SIZE 
                };
                DrawTexturePro(activeBrush->texture, activeBrush->sourceRect, dest, {0,0}, 0.0f, WHITE);
            }
        } else {
            Rectangle dest = { (float)obj.gridX * GRID_SIZE, (float)obj.gridY * GRID_SIZE, (float)GRID_SIZE, (float)GRID_SIZE };
            DrawRectangleRec(dest, MAGENTA);
            DrawRectangleLinesEx(dest, 1.0f, BLACK);
        }
    }
}

void RoomEditorState::DrawUI() {
    UIUtils::DrawPanel(sidebarBounds, Color{ 20, 20, 20, 240 });
    
    float startY = 20.0f;
    UIUtils::DrawCenteredText("PixeloidSans", "ROOM EDITOR", { sidebarBounds.width / 2.0f, startY }, UIUtils::FontSize::HEADER, WHITE);
    
    // Guide Button
    Rectangle guideBtn = { sidebarBounds.width - 40, 10, 30, 30 };
    Color guideColor = ORANGE;
    if (UIUtils::IsHovered(guideBtn)) {
        guideColor = Fade(guideColor, 0.8f);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            showGuide = true;
        }
    }
    DrawRectangleRec(guideBtn, guideColor);
    DrawRectangleLinesEx(guideBtn, 1.0f, BLACK);
    UIUtils::DrawCenteredText("PixeloidSans", "?", { guideBtn.x + guideBtn.width / 2.0f, guideBtn.y + guideBtn.height / 2.0f }, UIUtils::FontSize::HEADER, WHITE);
    
    startY += 50.0f;
    
    // Size Selection
    const char* sizes[] = { "SMALL", "MEDIUM", "LARGE" };
    for (int i = 0; i < 3; i++) {
        Rectangle btn = { 20, startY, sidebarBounds.width - 40, 30 };
        bool isSelected = static_cast<int>(currentRoomSize) == i;
        Color btnColor = isSelected ? ORANGE : LIGHTGRAY;
        
        if (UIUtils::IsHovered(btn)) {
            btnColor = Fade(btnColor, 0.8f);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentRoomSize = static_cast<RoomSize>(i);
                Initialize(); // Reset room
            }
        }
        DrawRectangleRec(btn, btnColor);
        DrawRectangleLinesEx(btn, 1.0f, BLACK);
        UIUtils::DrawCenteredText("PixeloidSans", sizes[i], { btn.x + btn.width / 2.0f, btn.y + btn.height / 2.0f }, UIUtils::FontSize::SMALL, BLACK);
        startY += 40.0f;
    }
    
    startY += 20.0f;
    
    // Bottom Buttons (Fixed at bottom)
    float bottomY = sidebarBounds.height - 110.0f;
    
    Rectangle exitBtn = { 20, bottomY, sidebarBounds.width - 40, 40 };
    Color exitColor = RED;
    if (UIUtils::IsHovered(exitBtn)) {
        exitColor = Fade(exitColor, 0.8f);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            GameManager::GetInstance().SetState(GameState::MAIN_MENU);
        }
    }
    DrawRectangleRec(exitBtn, exitColor);
    DrawRectangleLinesEx(exitBtn, 1.0f, BLACK);
    UIUtils::DrawCenteredText("PixeloidSans", "EXIT", { exitBtn.x + exitBtn.width / 2.0f, exitBtn.y + exitBtn.height / 2.0f }, UIUtils::FontSize::SMALL, WHITE);

    bottomY += 50.0f;
    
    Rectangle saveBtn = { 20, bottomY, sidebarBounds.width - 40, 40 };
    Color saveColor = SKYBLUE;
    if (UIUtils::IsHovered(saveBtn)) {
        saveColor = Fade(saveColor, 0.8f);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            std::string path = LevelIO::SaveRoomToCSV(currentRoomSize, placedObjects);
            if (!path.empty()) {
                statusMessage = "Export Successful: " + path;
            } else {
                statusMessage = "Export Failed!";
            }
            statusTimer = 3.0f;
        }
    }
    DrawRectangleRec(saveBtn, saveColor);
    DrawRectangleLinesEx(saveBtn, 1.0f, BLACK);
    UIUtils::DrawCenteredText("PixeloidSans", "SAVE CSV", { saveBtn.x + saveBtn.width / 2.0f, saveBtn.y + saveBtn.height / 2.0f }, UIUtils::FontSize::SMALL, BLACK);
    
    // Palette - Scissor Mode
    float panelHeight = sidebarBounds.height - 130.0f - startY; 
    BeginScissorMode(0, startY, sidebarBounds.width, panelHeight);
    
    float scrollY = startY + scrollOffset;
    std::string currentCategory = "";
    
    for (size_t i = 0; i < this->brushes.size(); i++) {
        const auto& brush = this->brushes[i];
        
        if (brush.category != currentCategory) {
            currentCategory = brush.category;
            scrollY += 10.0f;
            UIUtils::DrawCenteredText("PixeloidSans", currentCategory, { sidebarBounds.width / 2.0f, scrollY }, UIUtils::FontSize::SMALL, ORANGE);
            scrollY += 25.0f;
        }
        
        Rectangle btn = { 20, scrollY, sidebarBounds.width - 40, 45 };
        bool isSelected = static_cast<int>(currentBrush) == brush.objectID;
        Color btnColor = isSelected ? GREEN : LIGHTGRAY;
        
        if (UIUtils::IsHovered(btn) && CheckCollisionPointRec(GetMousePosition(), {0, startY, sidebarBounds.width, panelHeight})) {
            btnColor = Fade(btnColor, 0.8f);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentBrush = static_cast<BrushType>(brush.objectID);
            }
        }
        DrawRectangleRec(btn, btnColor);
        DrawRectangleLinesEx(btn, 1.0f, BLACK);
        
        if (brush.texture.id != 0) {
            Rectangle src = brush.sourceRect;
            float scaleX = 35.0f / src.width;
            float scaleY = 35.0f / src.height;
            float scale = std::min(scaleX, scaleY);
            
            Rectangle dest = { 
                btn.x + 5 + (35.0f - src.width * scale) / 2.0f, 
                btn.y + 5 + (35.0f - src.height * scale) / 2.0f, 
                src.width * scale, 
                src.height * scale 
            };
            DrawTexturePro(brush.texture, src, dest, {0,0}, 0.0f, WHITE);
        }
        
        UIUtils::DrawText("PixeloidSans", brush.name, { btn.x + 45, btn.y + 10 }, UIUtils::FontSize::SMALL, BLACK);
        UIUtils::DrawText("PixeloidSans", brush.sizeLabel, { btn.x + 45, btn.y + 25 }, UIUtils::FontSize::SMALL, DARKGRAY);
        scrollY += 50.0f;
    }
    
    EndScissorMode();
    
    // Draw status message
    if (statusTimer > 0.0f) {
        Color msgColor = (statusMessage.find("Successful") != std::string::npos) ? GREEN : RED;
        UIUtils::DrawCenteredText("PixeloidSans", statusMessage, { sidebarBounds.width + (GetScreenWidth() - sidebarBounds.width) / 2.0f, 40.0f }, UIUtils::FontSize::HEADER, msgColor);
    }
}

void RoomEditorState::DrawGuidePanel() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.8f));
    
    Rectangle panel = { GetScreenWidth() / 2.0f - 250, GetScreenHeight() / 2.0f - 200, 500, 400 };
    DrawRectangleRec(panel, DARKGRAY);
    DrawRectangleLinesEx(panel, 2.0f, ORANGE);
    
    float startY = panel.y + 30.0f;
    UIUtils::DrawCenteredText("PixeloidSans", "ROOM EDITOR GUIDE", { panel.x + panel.width / 2.0f, startY }, UIUtils::FontSize::HEADER, ORANGE);
    
    startY += 60.0f;
    UIUtils::DrawText("PixeloidSans", "- Pan Camera: W/A/S/D or Hold Middle Mouse", { panel.x + 40, startY }, UIUtils::FontSize::SMALL, WHITE);
    startY += 40.0f;
    UIUtils::DrawText("PixeloidSans", "- Zoom: Mouse Wheel (when outside sidebar)", { panel.x + 40, startY }, UIUtils::FontSize::SMALL, WHITE);
    startY += 40.0f;
    UIUtils::DrawText("PixeloidSans", "- Place Object: Left Click", { panel.x + 40, startY }, UIUtils::FontSize::SMALL, WHITE);
    startY += 40.0f;
    UIUtils::DrawText("PixeloidSans", "- Erase Object: Right Click", { panel.x + 40, startY }, UIUtils::FontSize::SMALL, WHITE);
    startY += 40.0f;
    UIUtils::DrawText("PixeloidSans", "- Scroll Palette: Mouse Wheel (hover sidebar)", { panel.x + 40, startY }, UIUtils::FontSize::SMALL, WHITE);
    
    startY += 80.0f;
    UIUtils::DrawCenteredText("PixeloidSans", "Click anywhere to close", { panel.x + panel.width / 2.0f, startY }, UIUtils::FontSize::SMALL, GRAY);
}
