#include "Entities/Wall.h"
#include "raylib.h"

Wall::Wall(Vector2 pos) : GameObject(pos) {
    // Each tile is 32x32 pixels
    boundingBox = { pos.x, pos.y, 32.0f, 32.0f };
}

void Wall::Update(float deltaTime) {
    // Walls are static, no update logic needed for now
}

void Wall::Draw() {
    // Draw wall as a solid DARKGRAY rectangle for now
    DrawRectangleRec(boundingBox, DARKGRAY);
    // Draw a border for visual clarity
    DrawRectangleLinesEx(boundingBox, 2.0f, BLACK);
}
