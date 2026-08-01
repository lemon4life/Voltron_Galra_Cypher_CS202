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

    // Debug Configurations
    constexpr bool DEBUG_DRAW_ENEMY_PATHS = true;

    // Scale & Transformation Constants
    constexpr float GLOBAL_SCALE = 1.5f; 
    constexpr Vector2 KNIGHT_PROJECTILE_OFFSET = { 27.0f, 4.0f };
}
