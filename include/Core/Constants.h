#pragma once
#include "raylib.h"

namespace Constants {
    // Window Configurations
    constexpr int SCREEN_WIDTH = 1280;
    constexpr int SCREEN_HEIGHT = 720;
    constexpr int GAME_WIDTH = 683; // Preserving internal resolution width for UI consistency
    constexpr int GAME_HEIGHT = 512; // Preserving internal resolution height for UI consistency
    inline const char* GAME_TITLE = "Voltron Mission - Galra Cypher";
    constexpr int TARGET_FPS = 60;
    
    // Game Settings
    inline bool isAutoAimEnabled = true;

    constexpr float GLOBAL_SCALE = 2.0f; 
    constexpr Vector2 KNIGHT_PROJECTILE_OFFSET = { 27.0f, 4.0f };

    // Dungeon Generation Constants
    constexpr int MAX_ROOM_TILE_SIZE = 32;
    constexpr int NORMAL_ROOM_TILE_SIZE = 20;
    constexpr int CORRIDOR_LENGTH = 5;
    constexpr int CORRIDOR_WIDTH = 5;
    constexpr float TILE_SIZE = 16.0f;
    constexpr float RENDER_TILE_SIZE = TILE_SIZE; // Changed to match character scale
}
