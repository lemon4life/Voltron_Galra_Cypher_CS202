#include "Entities/Hub/HubPaladinStand.h"

#include "raymath.h"

#include <cmath>

namespace {
constexpr int IDLE_FRAME_COUNT = 4;
constexpr float FRAME_DURATION = 0.1f;
constexpr float INTERACTION_DISTANCE = 44.0f;
}

HubPaladinStand::HubPaladinStand(
    PaladinId paladinId,
    Vector2 position,
    Texture2D idleTexture
)
    : GameObject(position, GameObjectType::HubPaladinStand),
      paladinId(paladinId),
      idleTexture(idleTexture),
      currentFrame(0),
      frameTimer(0.0f) {
}

void HubPaladinStand::Update(float deltaTime) {
    frameTimer += deltaTime;
    while (frameTimer >= FRAME_DURATION) {
        frameTimer -= FRAME_DURATION;
        currentFrame = (currentFrame + 1) % IDLE_FRAME_COUNT;
    }
}

void HubPaladinStand::Draw() {
    if (idleTexture.id == 0) {
        DrawRectangleRec(GetBoundingBox(), MAGENTA);
        return;
    }

    float frameWidth = (float)idleTexture.width / IDLE_FRAME_COUNT;
    float frameHeight = (float)idleTexture.height;
    Rectangle source = {
        currentFrame * frameWidth,
        0.0f,
        frameWidth,
        frameHeight
    };
    Rectangle destination = {
        std::round(position.x),
        std::round(position.y),
        frameWidth,
        frameHeight
    };
    Vector2 origin = {frameWidth * 0.5f, frameHeight * 0.5f};
    DrawTexturePro(
        idleTexture,
        source,
        destination,
        origin,
        0.0f,
        WHITE
    );
}

Rectangle HubPaladinStand::GetBoundingBox() const {
    return {position.x - 8.0f, position.y - 12.0f, 16.0f, 24.0f};
}

const char* HubPaladinStand::GetDisplayName() const {
    return PaladinCatalog::Get(paladinId).name.c_str();
}

bool HubPaladinStand::IsWithinInteractionRange(
    Vector2 playerPosition
) const {
    return Vector2Distance(playerPosition, position) <= INTERACTION_DISTANCE;
}
