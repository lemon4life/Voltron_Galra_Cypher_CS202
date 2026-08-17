#pragma once
#include "Entities/GameObject.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/AssetManager.h"

class Pot : public GameObject {
protected:
    bool isConsumed = false;
    Texture2D texture;
public:
    Pot(Vector2 pos, const char* textureKey) 
        : GameObject(pos, GameObjectType::Prop) {
        texture = AssetManager::GetInstance().GetTexture(textureKey);
        boundingBox = { pos.x - texture.width/2.0f, pos.y - texture.height/2.0f, (float)texture.width, (float)texture.height };
    }
    virtual ~Pot() = default;

    bool IsConsumed() const { return isConsumed; }
    virtual void OnConsume(TeamManager* team) = 0;

    void Update(float deltaTime) override {}
    void Draw() override {
        if (!isConsumed && texture.id != 0) {
            Vector2 origin = { texture.width / 2.0f, texture.height / 2.0f };
            Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
            Rectangle dest = { position.x, position.y, (float)texture.width, (float)texture.height };
            DrawTexturePro(texture, source, dest, origin, 0.0f, WHITE);
        }
    }
};

class HpPot : public Pot {
public:
    HpPot(Vector2 pos);
    void OnConsume(TeamManager* team) override;
};

class ExPot : public Pot {
public:
    ExPot(Vector2 pos);
    void OnConsume(TeamManager* team) override;
};

class QuintPot : public Pot {
public:
    QuintPot(Vector2 pos);
    void OnConsume(TeamManager* team) override;
};
