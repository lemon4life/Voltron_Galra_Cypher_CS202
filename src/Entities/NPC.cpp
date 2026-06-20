#include "Entities/NPC.h"

NPC::NPC(Vector2 pos) : GameObject(pos) {}

void NPC::Update(float deltaTime) {}

void NPC::Draw() {
    DrawRectangleRec(GetBoundingBox(), BLUE);
    DrawText("N", position.x - 4, position.y - 10, 20, WHITE);
}

Rectangle NPC::GetBoundingBox() const {
    return { position.x - 16.0f, position.y - 16.0f, 32.0f, 32.0f };
}
