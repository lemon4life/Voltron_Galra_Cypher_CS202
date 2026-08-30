#include "Entities/Wall.h"
#include "raylib.h"

/// Creates a Wall instance from the supplied configuration.
Wall::Wall(Vector2 pos) : GameObject(pos, GameObjectType::Wall) {
    // Each tile is 32x32 pixels
    boundingBox = { pos.x, pos.y, 32.0f, 32.0f };
}

/// Advances this component's state for the current frame.
void Wall::Update(float deltaTime) {
    // Walls are static, no update logic needed for now
}

/// Renders this component using its current state and visual resources.
void Wall::Draw() {
    // Draw wall as a solid DARKGRAY rectangle for now
    DrawRectangleRec(boundingBox, DARKGRAY);
    // Draw a border for visual clarity
    DrawRectangleLinesEx(boundingBox, 2.0f, BLACK);
}
