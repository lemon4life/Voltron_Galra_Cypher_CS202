#include "Entities/Props/EnhanceMachine.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/InputManager.h"
#include "Entities/Player/Paladin.h"
#include "UI/UIUtils.h"
#include "raymath.h"
#include <cmath>

EnhanceMachine::EnhanceMachine(Vector2 pos)
    : GameObject(pos, GameObjectType::Prop) {
    machineTexture = AssetManager::GetInstance().GetTexture("machine");
    boundingBox = { pos.x - 32.0f, pos.y - 36.0f, 64.0f, 72.0f };
}

void EnhanceMachine::Update(float deltaTime) {
    // 1. Continuous 4-frame animation (~7 FPS)
    frameTimer += deltaTime;
    if (frameTimer >= frameDuration) {
        frameTimer -= frameDuration;
        currentFrame = (currentFrame + 1) % numFrames;
    }

    // 2. Proximity & Interaction Check
    Paladin* paladin = GameManager::GetInstance().GetTeamManager() 
        ? GameManager::GetInstance().GetTeamManager()->GetActivePaladin() 
        : nullptr;

    if (paladin) {
        float dist = Vector2Distance(position, paladin->GetPosition());
        if (dist < 50.0f) {
            isPlayerInRange = true;
            if (InputManager::IsInteractPressed()) {
                OnInteract(paladin);
            }
        } else {
            isPlayerInRange = false;
        }
    } else {
        isPlayerInRange = false;
    }
}

void EnhanceMachine::OnInteract(Paladin* player) {
    if (!player) return;
    AudioManager::GetInstance().PlaySoundEffect("fx_button_click");

    // Open Enhance Machine Modal UI
    GameManager::GetInstance().OpenEnhanceMenu(player->GetPaladinId());
}

void EnhanceMachine::Draw() {
    if (machineTexture.id != 0) {
        Rectangle source = { (float)(currentFrame * 64), 0.0f, 64.0f, 72.0f };
        Rectangle dest = { std::round(position.x), std::round(position.y), 64.0f, 72.0f };
        Vector2 origin = { 32.0f, 36.0f };
        DrawTexturePro(machineTexture, source, dest, origin, 0.0f, WHITE);
    } else {
        DrawRectangle((int)(position.x - 32.0f), (int)(position.y - 36.0f), 64, 72, DARKBLUE);
    }
}

Rectangle EnhanceMachine::GetBoundingBox() const {
    return { position.x - 32.0f, position.y - 36.0f, 64.0f, 48.0f };
}

Rectangle EnhanceMachine::GetCollisionBox() const {
    return { position.x - 24.0f, position.y - 4.0f, 48.0f, 20.0f };
}
