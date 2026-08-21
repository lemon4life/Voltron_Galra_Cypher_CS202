#include "UI/MinimapRenderer.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Constants.h"

#include<cmath>

#include "Core/Manager/GameManager.h"

void MinimapRenderer::Draw(
    const LevelMap& levelMap,
    int currentGridX,
    int currentGridY,
    Rectangle bounds,
    int currentFloor
) {
    if (levelMap.grid.empty() || levelMap.generatedNodes.empty()) return;

    Vector2 anchor = { bounds.x, bounds.y };

    // Draw background panel
    DrawRectangleRec(
        { bounds.x - 2.0f, bounds.y - 2.0f,
          bounds.width + 4.0f, bounds.height + 4.0f },
        Fade(WHITE, 0.15f)
    );
    DrawRectangleRec(bounds, Fade(BLACK, 0.75f));
    DrawRectangleLinesEx(bounds, 1.5f, Fade(WHITE, 0.3f));

    float roomSize = 14.0f;
    float spacing = 22.0f;
    float corridorThickness = 4.0f;
    
    // Center the minimap view on the player's current room
    Vector2 centerMap = {
        bounds.x + bounds.width / 2.0f,
        bounds.y + bounds.height / 2.0f
    };
    
    auto checkRevealed = [](const RoomNode* n) {
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
        if (!checkRevealed(node.get())) continue;
        
        int dx = node->gridX - currentGridX;
        int dy = node->gridY - currentGridY;
        
        float nodeDrawX = centerMap.x + dx * spacing;
        float nodeDrawY = centerMap.y + dy * spacing;
        
        // Draw corridor to the EAST neighbor
        if (node->east && checkRevealed(node->east)) {
            float neighborDrawX = centerMap.x + (node->east->gridX - currentGridX) * spacing;
            float neighborDrawY = centerMap.y + (node->east->gridY - currentGridY) * spacing;
            
            // Corridor rectangle between the two rooms
            float corrX = nodeDrawX + roomSize / 2.0f;
            float corrY = nodeDrawY - corridorThickness / 2.0f;
            float corrW = neighborDrawX - roomSize / 2.0f - corrX;
            float corrH = corridorThickness;
            
            if (corrX >= bounds.x && corrX + corrW <= bounds.x + bounds.width &&
                corrY >= bounds.y && corrY + corrH <= bounds.y + bounds.height) {
                DrawRectangle(corrX, corrY, corrW, corrH, Fade(LIGHTGRAY, 0.5f));
            }
        }
        
        // Draw corridor to the SOUTH neighbor
        if (node->south && checkRevealed(node->south)) {
            float neighborDrawX = centerMap.x + (node->south->gridX - currentGridX) * spacing;
            float neighborDrawY = centerMap.y + (node->south->gridY - currentGridY) * spacing;
            
            float corrX = nodeDrawX - corridorThickness / 2.0f;
            float corrY = nodeDrawY + roomSize / 2.0f;
            float corrW = corridorThickness;
            float corrH = neighborDrawY - roomSize / 2.0f - corrY;
            
            if (corrX >= bounds.x && corrX + corrW <= bounds.x + bounds.width &&
                corrY >= bounds.y && corrY + corrH <= bounds.y + bounds.height) {
                DrawRectangle(corrX, corrY, corrW, corrH, Fade(LIGHTGRAY, 0.5f));
            }
        }
    }
    
    // --- PASS 2: Draw room squares on top ---
    for (const auto& node : levelMap.generatedNodes) {
        if (!checkRevealed(node.get())) continue;
        
        int dx = node->gridX - currentGridX;
        int dy = node->gridY - currentGridY;
        
        float drawX = centerMap.x + dx * spacing - roomSize / 2.0f;
        float drawY = centerMap.y + dy * spacing - roomSize / 2.0f;
        
        // Clip rooms outside the minimap bounds
        if (drawX < bounds.x - roomSize || drawX > bounds.x + bounds.width ||
            drawY < bounds.y - roomSize || drawY > bounds.y + bounds.height) {
            continue;
        }
        
        // Pick the room color based on type and state
        Color roomColor;
        Color borderColor;
        bool isCurrent = (dx == 0 && dy == 0);
        
        if (isCurrent) {
            roomColor = SKYBLUE;
            borderColor = BLUE;
        } else if (!node->isDiscovered) {
            roomColor = Fade(DARKGRAY, 0.9f);
            borderColor = DARKGRAY;
        } else if (node->state == RoomState::CLEARED) {
            roomColor = Fade(GRAY, 0.6f);
            borderColor = GRAY;
        } else {
            roomColor = Fade(LIGHTGRAY, 0.4f);
            borderColor = LIGHTGRAY;
        }
        
        // Draw the room square with border
        DrawRectangle(drawX, drawY, roomSize, roomSize, roomColor);
        DrawRectangleLinesEx({drawX, drawY, roomSize, roomSize}, 1.0f, borderColor);
        
        // Draw type icon inside the room
        float iconPad = 3.0f;
        float iconSize = roomSize - iconPad * 2;
        if (node->type == RoomType::SPAWN) {
            // Green inner square for spawn
            DrawRectangle(drawX + iconPad, drawY + iconPad, iconSize, iconSize, GREEN);
        } else if (node->type == RoomType::BOSS) {
            // Red inner square for boss
            DrawRectangle(drawX + iconPad, drawY + iconPad, iconSize, iconSize, RED);
        } else if (node->type == RoomType::CHEST) {
            // Gold inner square for chest
            DrawRectangle(drawX + iconPad, drawY + iconPad, iconSize, iconSize, GOLD);
        } else if (node->type == RoomType::BATTLE) {
            // Yellow ! for battle
            DrawRectangle(drawX + roomSize/2.0f - 1.0f, drawY + iconPad, 2.0f, iconSize - 3.0f, YELLOW);
            DrawRectangle(drawX + roomSize/2.0f - 1.0f, drawY + roomSize - iconPad - 1.0f, 2.0f, 2.0f, YELLOW);
        }
        
        // Pulsing indicator on the current room
        if (isCurrent) {
            float pulse = (sinf(GetTime() * 4.0f) + 1.0f) / 2.0f; // 0..1
            Color pulseColor = Fade(WHITE, 0.3f + 0.4f * pulse);
            DrawRectangleLinesEx({drawX - 2, drawY - 2, roomSize + 4, roomSize + 4}, 1.5f, pulseColor);
        }
    }
    
    // Draw minimap label    
    UIUtils::DrawText(
        "PixeloidSans",
        TextFormat("FLOOR: %d / %d", currentFloor, GameManager::MAX_FLOORS),
        { bounds.x + 4.0f, bounds.y + bounds.height - 14.0f },
        static_cast<UIUtils::FontSize>(10),
        Fade(WHITE, 0.5f)
    );
}
