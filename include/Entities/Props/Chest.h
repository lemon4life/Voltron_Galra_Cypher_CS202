#pragma once

#include "Entities/GameObject.h"
#include "Core/Manager/AssetManager.h"

class Chest : public GameObject {
private:
    bool isOpened = false;
    bool isOpening = false;
    float openProgress = 0.0f;     // 0.0f to 1.0f (sliding lid)
    float potScaleProgress = 0.0f; // 0.0f to 1.0f (pot scaling/emergence)
    int selectedPotType = 0;       // 0: HP, 1: EX, 2: Quintessence
    bool potSpawned = false;

    Texture2D chestBottom;
    Texture2D chestTop;

public:
    Chest(Vector2 pos);
    ~Chest() override = default;

    void Update(float deltaTime) override;
    void Draw() override;

    bool IsOpened() const { return isOpened; }
    bool IsOpening() const { return isOpening; }
    float GetOpenProgress() const { return openProgress; }
    float GetPotScaleProgress() const { return potScaleProgress; }
    int GetSelectedPotType() const { return selectedPotType; }
};
