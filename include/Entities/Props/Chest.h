#pragma once

#include "Entities/GameObject.h"
#include "Core/Manager/AssetManager.h"

enum class ChestRewardType {
    Pot,
    Coins,
    Cypher
};

class Chest : public GameObject {
private:
    ChestRewardType rewardType = ChestRewardType::Pot;
    bool isOpened = false;
    bool isOpening = false;
    bool cypherCollected = false;
    float openProgress = 0.0f;     // 0.0f to 1.0f (sliding lid)
    float potScaleProgress = 0.0f; // 0.0f to 1.0f (pot scaling/emergence)
    float cypherHoverTimer = 0.0f;
    int selectedPotType = 0;       // 0: HP, 1: EX, 2: Quintessence
    bool potSpawned = false;

    Texture2D chestBottom;
    Texture2D chestTop;

public:
    /// Creates a Chest instance from the supplied configuration.
    Chest(Vector2 pos, ChestRewardType reward = ChestRewardType::Pot);
    /// Releases resources owned by this Chest instance.
    ~Chest() override = default;

    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;

    /// Returns the current reward type.
    ChestRewardType GetRewardType() const { return rewardType; }
    /// Reports whether the opened condition is satisfied.
    bool IsOpened() const { return isOpened; }
    /// Reports whether the opening condition is satisfied.
    bool IsOpening() const { return isOpening; }
    /// Reports whether the cypher has been collected.
    bool IsCypherCollected() const { return cypherCollected; }
    /// Returns the current open progress.
    float GetOpenProgress() const { return openProgress; }
    /// Returns the current pot scale progress.
    float GetPotScaleProgress() const { return potScaleProgress; }
    /// Returns the current selected pot type.
    int GetSelectedPotType() const { return selectedPotType; }
};
