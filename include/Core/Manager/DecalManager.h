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

// Design Pattern - Singleton:
// DecalManager owns the shared persistent world-decal collection and exposes it
// through GetInstance; copy construction is disabled to prevent split ownership.
class DecalManager {
public:
    /// Returns the process-wide singleton instance of this manager.
    static DecalManager& GetInstance() {
        static DecalManager instance;
        return instance;
    }

    /// Adds corpse.
    void AddCorpse(Vector2 pos, Texture2D tex, bool facingLeft, Vector2 slideVel);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime, const LevelManager* levelManager);
    /// Renders this component using its current state and visual resources.
    void Draw();
    /// Removes all runtime entries owned by this component and resets transient state.
    void Clear();
    /// Returns the current count.
    std::size_t GetCount() const { return corpses.size(); }
    /// Returns the current capacity.
    std::size_t GetCapacity() const { return corpses.capacity(); }

private:
    /// Creates a DecalManager instance from the supplied configuration.
    DecalManager() = default;
    /// Releases resources owned by this DecalManager instance.
    ~DecalManager() = default;

    /// Creates a DecalManager instance from the supplied configuration.
    DecalManager(const DecalManager&) = delete;
    DecalManager& operator=(const DecalManager&) = delete;

    std::vector<CorpseDecal> corpses;
};
