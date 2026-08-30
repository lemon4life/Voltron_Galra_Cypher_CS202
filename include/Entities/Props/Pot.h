#pragma once
#include "Entities/GameObject.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/AssetManager.h"

class Pot : public GameObject {
protected:
    bool isConsumed = false;
    Texture2D texture;
public:
    /// Creates a Pot instance from the supplied configuration.
    Pot(Vector2 pos, const char* textureKey) 
        : GameObject(pos, GameObjectType::Prop) {
        texture = AssetManager::GetInstance().GetTexture(textureKey);
        boundingBox = { pos.x - texture.width/2.0f, pos.y - texture.height/2.0f, (float)texture.width, (float)texture.height };
    }
    /// Releases resources owned by this Pot instance.
    virtual ~Pot() = default;

    /// Reports whether the consumed condition is satisfied.
    bool IsConsumed() const { return isConsumed; }
    /// Handles the consume event.
    virtual void OnConsume(TeamManager* team) = 0;

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override {}
    /// Renders this component using its current state and visual resources.
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
    /// Creates a HpPot instance from the supplied configuration.
    HpPot(Vector2 pos);
    /// Handles the consume event.
    void OnConsume(TeamManager* team) override;
};

class ExPot : public Pot {
public:
    /// Creates a ExPot instance from the supplied configuration.
    ExPot(Vector2 pos);
    /// Handles the consume event.
    void OnConsume(TeamManager* team) override;
};

class QuintPot : public Pot {
public:
    /// Creates a QuintPot instance from the supplied configuration.
    QuintPot(Vector2 pos);
    /// Handles the consume event.
    void OnConsume(TeamManager* team) override;
};
