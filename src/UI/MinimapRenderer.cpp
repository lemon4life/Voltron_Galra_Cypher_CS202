#include "UI/MinimapRenderer.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Constants.h"
#include "Core/Level/RoomNode.h"
#include <cmath>
#include <algorithm>

void MinimapRenderer::Draw(
    const LevelMap& levelMap,
    int currentGridX,
    int currentGridY,
    Rectangle bounds,
    int currentFloor
) {
    if (levelMap.grid.empty() || levelMap.generatedNodes.empty()) return;

    // --- LAYER 1: Background Panel ---
    DrawRectangleRounded(bounds, 0.12f, 8, ColorAlpha(Color{ 10, 10, 15, 255 }, 0.8f));
    DrawRectangleRoundedLinesEx(bounds, 0.12f, 8, 1.0f, ColorAlpha(GRAY, 0.4f));

    // 1.5x Scaled Dimensions
    const float roomSize = 12.0f;           // 8 * 1.5
    const float iconSize = 9.0f;            // 6 * 1.5
    const float currentFrameSize = 15.0f;   // 10 * 1.5
    const float corridorThickness = 3.0f;   // 2 * 1.5
    const float spacing = 24.0f;            // 16 * 1.5

    // 1. Active-Room-Centric Panel Center (adjusted for bottom floor header)
    Vector2 panelCenter = {
        bounds.x + bounds.width * 0.5f,
        bounds.y + (bounds.height - 20.0f) * 0.5f + 2.0f
    };

    // Calculate any room (gx, gy)'s center relative to the active room
    auto getNodeCenter = [&](int gx, int gy) -> Vector2 {
        return {
            panelCenter.x + (gx - currentGridX) * spacing,
            panelCenter.y + (gy - currentGridY) * spacing
        };
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

    // Calculate screen-space coordinates for Raylib's BeginScissorMode
    float viewportScale = std::min(
        (float)GetScreenWidth() / (float)Constants::GAME_WIDTH,
        (float)GetScreenHeight() / (float)Constants::GAME_HEIGHT
    );
    Vector2 cameraOffset = {
        (GetScreenWidth()  - (float)Constants::GAME_WIDTH  * viewportScale) * 0.5f,
        (GetScreenHeight() - (float)Constants::GAME_HEIGHT * viewportScale) * 0.5f
    };

    int scissorX = (int)std::round((bounds.x + 2.0f) * viewportScale + cameraOffset.x);
    int scissorY = (int)std::round((bounds.y + 2.0f) * viewportScale + cameraOffset.y);
    int scissorW = (int)std::round((bounds.width - 4.0f) * viewportScale);
    int scissorH = (int)std::round((bounds.height - 4.0f) * viewportScale);

    // --- LAYER 2: Scissor Masked Drawing (Crops anything extending past panel bounds) ---
    BeginScissorMode(scissorX, scissorY, scissorW, scissorH);

    // Pass 1: Connecting Corridors Behind Nodes
    for (const auto& node : levelMap.generatedNodes) {
        if (!checkRevealed(node)) continue;
        
        Vector2 c1 = getNodeCenter(node->gridX, node->gridY);
        
        // Corridor to EAST neighbor
        if (node->east && checkRevealed(node->east)) {
            Vector2 c2 = getNodeCenter(node->east->gridX, node->east->gridY);
            
            float corrX = c1.x + roomSize * 0.5f;
            float corrY = c1.y - corridorThickness * 0.5f;
            float corrW = (c2.x - roomSize * 0.5f) - corrX;
            float corrH = corridorThickness;

            Color corrColor = (node->isDiscovered && node->east->isDiscovered)
                ? Color{ 140, 140, 150, 200 }
                : Color{ 70, 70, 75, 200 };
            DrawRectangle((int)corrX, (int)corrY, (int)corrW, (int)corrH, corrColor);
        }
        
        // Corridor to SOUTH neighbor
        if (node->south && checkRevealed(node->south)) {
            Vector2 c2 = getNodeCenter(node->south->gridX, node->south->gridY);
            
            float corrX = c1.x - corridorThickness * 0.5f;
            float corrY = c1.y + roomSize * 0.5f;
            float corrW = corridorThickness;
            float corrH = (c2.y - roomSize * 0.5f) - corrY;

            Color corrColor = (node->isDiscovered && node->south->isDiscovered)
                ? Color{ 140, 140, 150, 200 }
                : Color{ 70, 70, 75, 200 };
            DrawRectangle((int)corrX, (int)corrY, (int)corrW, (int)corrH, corrColor);
        }
    }

    // Pass 2: Base Nodes (12x12) & Room Type Icons (9x9)
    Vector2 activeNodeCenter = { 0.0f, 0.0f };
    bool hasActiveNode = false;

    for (const auto& node : levelMap.generatedNodes) {
        if (!checkRevealed(node)) continue;
        
        Vector2 nodeCenter = getNodeCenter(node->gridX, node->gridY);

        bool isCurrent = (node->gridX == currentGridX && node->gridY == currentGridY);
        if (isCurrent) {
            activeNodeCenter = nodeCenter;
            hasActiveNode = true;
        }

        // Base 12x12 node centered at nodeCenter (offset -6.0f, -6.0f)
        Color nodeColor = node->isDiscovered
            ? Color{ 140, 140, 150, 255 }
            : Color{ 70, 70, 75, 255 };

        DrawRectangle(
            (int)(nodeCenter.x - 6.0f),
            (int)(nodeCenter.y - 6.0f),
            (int)roomSize,
            (int)roomSize,
            nodeColor
        );

        // Room Type Icon (9x9 centered at nodeCenter: offset -4.5f, -4.5f)
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
                iconKey = nullptr; // Blank base 12x12 node
                break;
        }

        if (iconKey != nullptr) {
            Texture2D icon = AssetManager::GetInstance().GetTexture(iconKey);
            if (icon.id != 0) {
                Rectangle src = { 0.0f, 0.0f, (float)icon.width, (float)icon.height };
                Rectangle dest = {
                    nodeCenter.x - 4.5f,
                    nodeCenter.y - 4.5f,
                    iconSize,
                    iconSize
                };
                DrawTexturePro(icon, src, dest, { 0.0f, 0.0f }, 0.0f, WHITE);
            }
        }
    }

    // Pass 3: Active Room Outer Frame (15x15)
    if (hasActiveNode) {
        Texture2D currentTex = AssetManager::GetInstance().GetTexture("minimap_current");
        if (currentTex.id != 0) {
            Rectangle src = { 0.0f, 0.0f, (float)currentTex.width, (float)currentTex.height };
            Rectangle dest = {
                activeNodeCenter.x - 7.5f,
                activeNodeCenter.y - 7.5f,
                currentFrameSize,
                currentFrameSize
            };
            DrawTexturePro(currentTex, src, dest, { 0.0f, 0.0f }, 0.0f, WHITE);
        }
    }

    EndScissorMode();

    // --- LAYER 3 (Outside Scissor / Top): Floor Header Text ---
    UIUtils::DrawText(
        "PixeloidSans",
        TextFormat("FLOOR %d", currentFloor),
        { bounds.x + 8.0f, bounds.y + bounds.height - 18.0f },
        static_cast<UIUtils::FontSize>(10),
        Color{ 200, 200, 200, 255 }
    );
}
