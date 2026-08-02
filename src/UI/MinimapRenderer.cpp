#include "UI/MinimapRenderer.h"
#include "Core/Constants.h"

void MinimapRenderer::Draw(const LevelMap& levelMap, int currentGridX, int currentGridY) {
    if (levelMap.grid.empty() || levelMap.generatedNodes.empty()) return;

    float minimapSize = 150.0f;
    float padding = 10.0f;
    Vector2 anchor = { Constants::GAME_WIDTH - minimapSize - padding, padding };
    
    // Draw background panel
    DrawRectangle(anchor.x - 2, anchor.y - 2, minimapSize + 4, minimapSize + 4, Fade(WHITE, 0.15f));
    DrawRectangle(anchor.x, anchor.y, minimapSize, minimapSize, Fade(BLACK, 0.75f));
    DrawRectangleLinesEx({anchor.x, anchor.y, minimapSize, minimapSize}, 1.5f, Fade(WHITE, 0.3f));

    float roomSize = 14.0f;
    float spacing = 22.0f;
    float corridorThickness = 4.0f;
    
    // Center the minimap view on the player's current room
    Vector2 centerMap = { anchor.x + minimapSize / 2.0f, anchor.y + minimapSize / 2.0f };
    
    // --- PASS 1: Draw corridors (connections) behind rooms ---
    for (const auto& node : levelMap.generatedNodes) {
        if (!node->isDiscovered) continue;
        
        int dx = node->gridX - currentGridX;
        int dy = node->gridY - currentGridY;
        
        float nodeDrawX = centerMap.x + dx * spacing;
        float nodeDrawY = centerMap.y + dy * spacing;
        
        // Draw corridor to the EAST neighbor
        if (node->east && node->east->isDiscovered) {
            float neighborDrawX = centerMap.x + (node->east->gridX - currentGridX) * spacing;
            float neighborDrawY = centerMap.y + (node->east->gridY - currentGridY) * spacing;
            
            // Corridor rectangle between the two rooms
            float corrX = nodeDrawX + roomSize / 2.0f;
            float corrY = nodeDrawY - corridorThickness / 2.0f;
            float corrW = neighborDrawX - roomSize / 2.0f - corrX;
            float corrH = corridorThickness;
            
            if (corrX >= anchor.x && corrX + corrW <= anchor.x + minimapSize &&
                corrY >= anchor.y && corrY + corrH <= anchor.y + minimapSize) {
                DrawRectangle(corrX, corrY, corrW, corrH, Fade(LIGHTGRAY, 0.5f));
            }
        }
        
        // Draw corridor to the SOUTH neighbor
        if (node->south && node->south->isDiscovered) {
            float neighborDrawX = centerMap.x + (node->south->gridX - currentGridX) * spacing;
            float neighborDrawY = centerMap.y + (node->south->gridY - currentGridY) * spacing;
            
            float corrX = nodeDrawX - corridorThickness / 2.0f;
            float corrY = nodeDrawY + roomSize / 2.0f;
            float corrW = corridorThickness;
            float corrH = neighborDrawY - roomSize / 2.0f - corrY;
            
            if (corrX >= anchor.x && corrX + corrW <= anchor.x + minimapSize &&
                corrY >= anchor.y && corrY + corrH <= anchor.y + minimapSize) {
                DrawRectangle(corrX, corrY, corrW, corrH, Fade(LIGHTGRAY, 0.5f));
            }
        }
    }
    
    // --- PASS 2: Draw room squares on top ---
    for (const auto& node : levelMap.generatedNodes) {
        if (!node->isDiscovered) continue;
        
        int dx = node->gridX - currentGridX;
        int dy = node->gridY - currentGridY;
        
        float drawX = centerMap.x + dx * spacing - roomSize / 2.0f;
        float drawY = centerMap.y + dy * spacing - roomSize / 2.0f;
        
        // Clip rooms outside the minimap bounds
        if (drawX < anchor.x - roomSize || drawX > anchor.x + minimapSize ||
            drawY < anchor.y - roomSize || drawY > anchor.y + minimapSize) {
            continue;
        }
        
        // Pick the room color based on type and state
        Color roomColor;
        Color borderColor;
        bool isCurrent = (dx == 0 && dy == 0);
        
        if (isCurrent) {
            roomColor = WHITE;
            borderColor = WHITE;
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
        }
        
        // Pulsing indicator on the current room
        if (isCurrent) {
            float pulse = (sinf(GetTime() * 4.0f) + 1.0f) / 2.0f; // 0..1
            Color pulseColor = Fade(WHITE, 0.3f + 0.4f * pulse);
            DrawRectangleLinesEx({drawX - 2, drawY - 2, roomSize + 4, roomSize + 4}, 1.5f, pulseColor);
        }
    }
    
    // Draw minimap label
    DrawText("MAP", anchor.x + 4, anchor.y + minimapSize - 14, 10, Fade(WHITE, 0.5f));
}
