#include "Entities/NPC.h"
#include "Core/Manager/AssetManager.h"
#include "UI/UIUtils.h"
#include <cmath>

/// Creates a NPC instance from the supplied configuration.
NPC::NPC(Vector2 pos, NpcId id)
    : GameObject(pos, GameObjectType::NPC),
      npcId(id),
      currentFrame(0),
      frameTimer(0.0f),
      frameDuration(0.1f),
      numFrames(4) {
    AssetManager::GetInstance().LoadTexture2D(
        "NPC_Allura_Idle",
        "assets/sprites/Allura/Idle_Sheet.png",
        true
    );
    AssetManager::GetInstance().LoadTexture2D(
        "NPC_Shiro_Idle",
        "assets/sprites/Shiro/Idle_Sheet.png",
        true
    );
}

/// Advances this component's state for the current frame.
void NPC::Update(float deltaTime) {
    frameTimer += deltaTime;
    if (frameTimer >= frameDuration) {
        frameTimer -= frameDuration;
        currentFrame = (currentFrame + 1) % numFrames;
    }
}

/// Renders this component using its current state and visual resources.
void NPC::Draw() {
    Texture2D tex = AssetManager::GetInstance().GetTexture(
        npcId == NpcId::Shiro ? "NPC_Shiro_Idle" : "NPC_Allura_Idle"
    );
    
    if (tex.id != 0) {
        int frameWidthI  = tex.width / numFrames;
        float frameWidth  = (float)frameWidthI;
        float frameHeight = (float)tex.height;

        Rectangle source = { (float)(currentFrame * frameWidthI), 0.0f, frameWidth, frameHeight };
        Rectangle dest   = { std::round(position.x), std::round(position.y), frameWidth, frameHeight };
        Vector2 origin   = { frameWidth / 2.0f, frameHeight / 2.0f };
        DrawTexturePro(tex, source, dest, origin, 0.0f, WHITE);
    } else {
        DrawRectangleRec(GetBoundingBox(), BLUE);
        UIUtils::DrawText("PixeloidBold", "N", { position.x - 4, position.y - 10 }, UIUtils::FontSize::BODY, WHITE);
    }
}

/// Returns the current bounding box.
Rectangle NPC::GetBoundingBox() const {
    return { position.x - 16.0f, position.y - 16.0f, 32.0f, 32.0f };
}
