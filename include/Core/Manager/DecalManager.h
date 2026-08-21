#pragma once

#include "raylib.h"
#include <vector>

class LevelManager;

struct CorpseDecal {
    Vector2 position;
    Texture2D texture;
    bool facingLeft;
    float heightOffset;     // Simulated Z-axis (negative is up)
    float verticalVelocity; 
    Vector2 slideVelocity;
    bool settled;           // Stops updating once finished bouncing
    float age = 0.0f;
};

class DecalManager {
public:
    static DecalManager& GetInstance() {
        static DecalManager instance;
        return instance;
    }

    void AddCorpse(Vector2 pos, Texture2D tex, bool facingLeft, Vector2 slideVel);
    void Update(float deltaTime, const LevelManager* levelManager);
    void Draw();
    void Clear();
    std::size_t GetCount() const { return corpses.size(); }
    std::size_t GetCapacity() const { return corpses.capacity(); }

private:
    DecalManager() = default;
    ~DecalManager() = default;

    DecalManager(const DecalManager&) = delete;
    DecalManager& operator=(const DecalManager&) = delete;

    std::vector<CorpseDecal> corpses;
};
