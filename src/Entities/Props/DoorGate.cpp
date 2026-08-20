#include "Entities/Props/DoorGate.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Constants.h"

DoorGate::DoorGate(Vector2 pos) 
    : MapObject(
          pos,
          { pos.x, pos.y, Constants::RENDER_TILE_SIZE,
            Constants::RENDER_TILE_SIZE },
          { -1, -1 },
          MapObjectId::Empty
      ),
      state(State::OPEN),
      currentFrame(0),
      animationTimer(0.0f),
      frameDuration(0.05f), // 0.05s per frame
      totalFrames(8) 
{
    boundingBox = {pos.x, pos.y, Constants::RENDER_TILE_SIZE, Constants::RENDER_TILE_SIZE};
    tex = AssetManager::GetInstance().GetTexture("doorGate");
    if (tex.id == 0) {
        tex = AssetManager::GetInstance().LoadTexture2D("doorGate", "assets/tileset/Galra_Door_8.png", true);
    }
}

void DoorGate::SetState(State newState) {
    if (state == newState) return;
    
    state = newState;
    if (state == State::OPEN) {
        currentFrame = 0;
    } else if (state == State::LOCKED) {
        currentFrame = totalFrames - 1;
    } else if (state == State::CLOSING) {
        // Keep currentFrame if it was OPENING, just reverse
        if (currentFrame == totalFrames - 1) currentFrame = 0;
        animationTimer = 0.0f;
    } else if (state == State::OPENING) {
        if (currentFrame == 0) currentFrame = totalFrames - 1;
        animationTimer = 0.0f;
    }
}

void DoorGate::Update(float deltaTime) {
    if (state == State::CLOSING) {
        animationTimer += deltaTime;
        if (animationTimer >= frameDuration) {
            animationTimer -= frameDuration;
            currentFrame++;
            if (currentFrame >= totalFrames - 1) {
                currentFrame = totalFrames - 1;
                state = State::LOCKED;
            }
        }
    } else if (state == State::OPENING) {
        animationTimer += deltaTime;
        if (animationTimer >= frameDuration) {
            animationTimer -= frameDuration;
            currentFrame--;
            if (currentFrame <= 0) {
                currentFrame = 0;
                state = State::OPEN;
            }
        }
    }
}

Rectangle DoorGate::GetBoundingBox() const {
    return boundingBox;
}

void DoorGate::DrawBaseLayer() {
    if (tex.id == 0) return;
    
    float frameWidth = (float)tex.width / totalFrames;
    float frameHeight = (float)tex.height;
    float splitY = frameHeight * 0.5f;

    if (state == State::LOCKED) {
        // Bottom half of the sprite
        Rectangle src = {currentFrame * frameWidth, splitY, frameWidth, frameHeight - splitY};
        
        // The bottom half exactly overlays the bounding box, visually shifted down by 1 tile
        Rectangle dest = boundingBox;
        dest.y += Constants::RENDER_TILE_SIZE;
        DrawTexturePro(tex, src, dest, {0,0}, 0.0f, WHITE);
    } else if (state == State::OPEN) {
        // Draw whole sprite in base layer
        Rectangle src = {currentFrame * frameWidth, 0.0f, frameWidth, frameHeight};
        Rectangle dest = {boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height * 2.0f};
        DrawTexturePro(tex, src, dest, {0,0}, 0.0f, WHITE);
    }
}

void DoorGate::AddDepthRenderItems(std::vector<DepthRenderItem>& items) {
    if (state == State::OPEN) return; // No top half to render if open
    
    items.push_back({
        boundingBox.y + boundingBox.height,
        [this]() {
            if (tex.id == 0) return;
            
            float frameWidth = (float)tex.width / totalFrames;
            float frameHeight = (float)tex.height;
            float splitY = frameHeight * 0.5f;

            if (state == State::LOCKED) {
                // Top half of the sprite
                Rectangle src = {currentFrame * frameWidth, 0.0f, frameWidth, splitY};
                
                // Render directly above the visual bounding box
                Rectangle dest = {boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height};
                DrawTexturePro(tex, src, dest, {0,0}, 0.0f, WHITE);
            } else {
                // CLOSING or OPENING: Draw whole sprite, no splitting
                Rectangle src = {currentFrame * frameWidth, 0.0f, frameWidth, frameHeight};
                Rectangle dest = {boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height * 2.0f};
                DrawTexturePro(tex, src, dest, {0,0}, 0.0f, WHITE);
            }
        }
    });
}
