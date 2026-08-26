#include "UI/MinimapRenderer.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Constants.h"
#include "Core/Level/RoomNode.h"
#include <cmath>

void MinimapRenderer::Draw(
    const LevelMap& levelMap,
    int currentGridX,
    int currentGridY,
    Rectangle bounds,
    int currentFloor
) {
    if (levelMap.grid.empty() || levelMap.generatedNodes.empty()) return;

    // 1. Base Panel: Dark rounded rectangle with 80% opacity
    DrawRectangleRounded(bounds, 0.12f, 8, ColorAlpha(Color{ 10, 10, 15, 255 }, 0.8f));
    DrawRectangleRoundedLinesEx(bounds, 0.12f, 8, 1.0f, ColorAlpha(GRAY, 0.4f));

    // 2. Floor Indicator: Render current floor in dim white
    UIUtils::DrawText(
        "PixeloidSans",
        TextFormat("FLOOR %d", currentFloor),
        { bounds.x + 8.0f, bounds.y + bounds.height - 18.0f },
        static_cast<UIUtils::FontSize>(10),
        Color{ 200, 200, 200, 255 }
    );

    // 2x Scaled Grid Dimensions
    const float roomSize = 16.0f;
    const float spacing = 32.0f;
    const float corridorThickness = 4.0f;
    
    // Center the minimap view on the player's current room
    Vector2 centerMap = {
        bounds.x + bounds.width / 2.0f,
        bounds.y + bounds.height / 2.0f
    };
    
    auto checkRevealed = [](const std::shared_ptr<RoomNode>& n) {
        if (!n) return false;
        if (n->isDiscovered) return true;
        if (n->north && n->north->isDiscovered) return true;
        if (n->south && n->south->isDiscovered) return true;
        if (n->east && n->east->isDiscovered) return true;
        if (n->west && n->west->isDiscovered) return true;
        return false;
    };
    
    // --- PASS 1: Draw corridors (connections) behind rooms ---
    for (const auto& node : levelMap.generatedNodes) {
        if (!checkRevealed(node)) continue;
        
        int dx = node->gridX - currentGridX;
        int dy = node->gridY - currentGridY;
        
        float nodeDrawX = centerMap.x + dx * spacing - roomSize / 2.0f;
        float nodeDrawY = centerMap.y + dy * spacing - roomSize / 2.0f;
        
        // Draw corridor to the EAST neighbor
        if (node->east && checkRevealed(node->east)) {
            float neighborDrawX = centerMap.x + (node->east->gridX - currentGridX) * spacing - roomSize / 2.0f;
            
            float corrX = nodeDrawX + roomSize;
            float corrY = nodeDrawY + (roomSize - corridorThickness) / 2.0f;
            float corrW = neighborDrawX - corrX;
            float corrH = corridorThickness;
            
            if (corrX >= bounds.x && corrX + corrW <= bounds.x + bounds.width &&
                corrY >= bounds.y && corrY + corrH <= bounds.y + bounds.height) {
                Color corrColor = (node->isDiscovered && node->east->isDiscovered)
                    ? Color{ 140, 140, 150, 200 }
                    : Color{ 70, 70, 75, 200 };
                DrawRectangle((int)corrX, (int)corrY, (int)corrW, (int)corrH, corrColor);
            }
        }
        
        // Draw corridor to the SOUTH neighbor
        if (node->south && checkRevealed(node->south)) {
            float neighborDrawY = centerMap.y + (node->south->gridY - currentGridY) * spacing - roomSize / 2.0f;
            
            float corrX = nodeDrawX + (roomSize - corridorThickness) / 2.0f;
            float corrY = nodeDrawY + roomSize;
            float corrW = corridorThickness;
            float corrH = neighborDrawY - corrY;
            
            if (corrX >= bounds.x && corrX + corrW <= bounds.x + bounds.width &&
                corrY >= bounds.y && corrY + corrH <= bounds.y + bounds.height) {
                Color corrColor = (node->isDiscovered && node->south->isDiscovered)
                    ? Color{ 140, 140, 150, 200 }
                    : Color{ 70, 70, 75, 200 };
                DrawRectangle((int)corrX, (int)corrY, (int)corrW, (int)corrH, corrColor);
            }
        }
    }
    
    // --- PASS 2: Draw room base nodes (16x16) and 12x12 icons ---
    float currentRoomDrawX = 0.0f;
    float currentRoomDrawY = 0.0f;
    bool hasCurrentRoom = false;

    for (const auto& node : levelMap.generatedNodes) {
        if (!checkRevealed(node)) continue;
        
        int dx = node->gridX - currentGridX;
        int dy = node->gridY - currentGridY;
        
        float drawX = centerMap.x + dx * spacing - roomSize / 2.0f;
        float drawY = centerMap.y + dy * spacing - roomSize / 2.0f;
        
        // Clip rooms outside the minimap bounds
        if (drawX < bounds.x - roomSize || drawX > bounds.x + bounds.width ||
            drawY < bounds.y - roomSize || drawY > bounds.y + bounds.height) {
            continue;
        }

        bool isCurrent = (dx == 0 && dy == 0);
        if (isCurrent) {
            currentRoomDrawX = drawX;
            currentRoomDrawY = drawY;
            hasCurrentRoom = true;
        }

        // Base 16x16 node color:
        // Unvisited Rooms: Medium-dark gray (Color{70, 70, 75, 255})
        // Visited Rooms: Light gray (Color{140, 140, 150, 255})
        Color nodeColor = node->isDiscovered
            ? Color{ 140, 140, 150, 255 }
            : Color{ 70, 70, 75, 255 };

        DrawRectangle((int)drawX, (int)drawY, (int)roomSize, (int)roomSize, nodeColor);

        // Room Icon Mapping (12x12 centered inside 16x16 node -> offset +2, +2)
        const char* iconKey = nullptr;
        switch (node->type) {
            case RoomType::SPAWN:
                iconKey = "minimap_home";
                break;
            case RoomType::EVENT:
            case RoomType::CHEST:
                iconKey = "minimap_event";
                break;
            case RoomType::EXIT:
                iconKey = "minimap_exit";
                break;
            case RoomType::BOSS:
                iconKey = "minimap_boss";
                break;
            case RoomType::BATTLE:
            default:
                iconKey = nullptr; // Blank base 16x16 node
                break;
        }

        if (iconKey != nullptr) {
            Texture2D icon = AssetManager::GetInstance().GetTexture(iconKey);
            if (icon.id != 0) {
                Rectangle src = { 0.0f, 0.0f, (float)icon.width, (float)icon.height };
                Rectangle dest = { drawX + 2.0f, drawY + 2.0f, 12.0f, 12.0f };
                DrawTexturePro(icon, src, dest, { 0.0f, 0.0f }, 0.0f, WHITE);
            }
        }
    }

    // --- PASS 3: Active Room Outer Corner Indicator ---
    // Draw minimap_current (20x20) centered over the active 16x16 node (drawX - 2, drawY - 2)
    if (hasCurrentRoom) {
        Texture2D currentTex = AssetManager::GetInstance().GetTexture("minimap_current");
        if (currentTex.id != 0) {
            Rectangle src = { 0.0f, 0.0f, (float)currentTex.width, (float)currentTex.height };
            Rectangle dest = { currentRoomDrawX - 2.0f, currentRoomDrawY - 2.0f, 20.0f, 20.0f };
            DrawTexturePro(currentTex, src, dest, { 0.0f, 0.0f }, 0.0f, WHITE);
        }
    }
}
