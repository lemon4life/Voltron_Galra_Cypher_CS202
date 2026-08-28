#include "Entities/Items/Coin.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/ObjectManager.h"
#include "Entities/Player/Paladin.h"
#include "raymath.h"
#include <cmath>

Coin::Coin(Vector2 pos, Vector2 initialVelocity)
    : GameObject(pos, GameObjectType::Prop),
      velocity(initialVelocity),
      burstTimer(0.25f),
      isCollected(false) {
    texture = AssetManager::GetInstance().GetTexture("coin_world");
    boundingBox = { pos.x - 4.0f, pos.y - 4.5f, 8.0f, 9.0f };
}

void Coin::Update(float deltaTime) {
    if (isCollected) return;

    // 1. 4-frame animation (~8 FPS)
    frameTimer += deltaTime;
    if (frameTimer >= frameDuration) {
        frameTimer -= frameDuration;
        currentFrame = (currentFrame + 1) % numFrames;
    }

    // 2. Physics & Magnetism
    Paladin* paladin = GameManager::GetInstance().GetTeamManager()
        ? GameManager::GetInstance().GetTeamManager()->GetActivePaladin()
        : nullptr;

    if (paladin) {
        Vector2 paladinPos = paladin->GetPosition();
        float dist = Vector2Distance(position, paladinPos);

        if (burstTimer > 0.0f) {
            burstTimer -= deltaTime;
            velocity = Vector2Scale(velocity, friction);
        } else if (dist < 120.0f) {
            Vector2 dir = Vector2Normalize(Vector2Subtract(paladinPos, position));
            Vector2 targetVel = Vector2Scale(dir, magnetSpeed);
            velocity = Vector2Lerp(velocity, targetVel, deltaTime * 10.0f);
        } else {
            velocity = Vector2Scale(velocity, friction);
        }

        position = Vector2Add(position, Vector2Scale(velocity, deltaTime));
        boundingBox = { position.x - 4.0f, position.y - 4.5f, 8.0f, 9.0f };

        // 3. Collection check
        if (dist < 16.0f) {
            isCollected = true;
            if (auto* tm = GameManager::GetInstance().GetTeamManager()) {
                tm->AddCoins(1);
            }
            AudioManager::GetInstance().PlaySoundEffect("fx_coin");
            GameManager::GetInstance().GetObjectManager().QueueRemoval(this);
        }
    } else {
        position = Vector2Add(position, Vector2Scale(velocity, deltaTime));
        velocity = Vector2Scale(velocity, friction);
        boundingBox = { position.x - 4.0f, position.y - 4.5f, 8.0f, 9.0f };
    }
}

void Coin::Draw() {
    if (isCollected) return;

    if (texture.id != 0) {
        Rectangle source = { (float)(currentFrame * 8), 0.0f, 8.0f, 9.0f };
        Rectangle dest = { std::round(position.x), std::round(position.y), 8.0f, 9.0f };
        Vector2 origin = { 4.0f, 4.5f };
        DrawTexturePro(texture, source, dest, origin, 0.0f, WHITE);
    } else {
        DrawCircle((int)position.x, (int)position.y, 4.0f, GOLD);
    }
}
