#include "Entities/NPC.h"
#include "UI/UIUtils.h"
#include <cmath>

static Texture2D alluraSprite = { 0 };

NPC::NPC(Vector2 pos)
    : GameObject(pos, GameObjectType::NPC),
      currentFrame(0),
      frameTimer(0.0f),
      frameDuration(0.1f),
      numFrames(4) {
    if (alluraSprite.id == 0) {
        alluraSprite = LoadTexture("assets/sprites/Allura/Idle_Sheet.png");
        SetTextureFilter(alluraSprite, TEXTURE_FILTER_POINT);
    }
}

void NPC::Update(float deltaTime) {
    frameTimer += deltaTime;
    if (frameTimer >= frameDuration) {
        frameTimer -= frameDuration;
        currentFrame = (currentFrame + 1) % numFrames;
    }
}

void NPC::Draw() {
    if (alluraSprite.id != 0) {
        int frameWidthI  = alluraSprite.width / numFrames;   // integer — no fractional source rect
        float frameWidth  = (float)frameWidthI;
        float frameHeight = (float)alluraSprite.height;

        Rectangle source = { (float)(currentFrame * frameWidthI), 0.0f, frameWidth, frameHeight };
        Rectangle dest   = { std::round(position.x), std::round(position.y), frameWidth, frameHeight };
        Vector2 origin   = { frameWidth / 2.0f, frameHeight / 2.0f };
        DrawTexturePro(alluraSprite, source, dest, origin, 0.0f, WHITE);
    } else {
        DrawRectangleRec(GetBoundingBox(), BLUE);
        UIUtils::DrawText("PixeloidBold", "N", { position.x - 4, position.y - 10 }, UIUtils::FontSize::BODY, WHITE);
    }
}

Rectangle NPC::GetBoundingBox() const {
    return { position.x - 16.0f, position.y - 16.0f, 32.0f, 32.0f };
}
