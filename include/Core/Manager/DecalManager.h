#pragma once

#include "raylib.h"
#include <vector>

struct CorpseDecal {
    Vector2 position;
    Texture2D texture;
    bool facingLeft;
    float heightOffset;     // Simulated Z-axis (negative is up)
    float verticalVelocity; 
    Vector2 slideVelocity;
    bool settled;           // Stops updating once finished bouncing
};

class DecalManager {
public:
    static DecalManager& GetInstance() {
        static DecalManager instance;
        return instance;
    }

    void AddCorpse(Vector2 pos, Texture2D tex, bool facingLeft, Vector2 slideVel);
    void Update(float deltaTime);
    void Draw();
    void Clear();

private:
    DecalManager() = default;
    ~DecalManager() = default;

    DecalManager(const DecalManager&) = delete;
    DecalManager& operator=(const DecalManager&) = delete;

    std::vector<CorpseDecal> corpses;
};
