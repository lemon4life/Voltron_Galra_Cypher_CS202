#pragma once

#include "Core/LevelAccess.h"
#include "Entities/GameObject.h"

class DestructibleBox final : public GameObject {
private:
    IMapObjectDestroyAccess& destroyAccess;
    const GameObjectCell objectCell;
    int health;
    bool destructionQueued;

public:
    DestructibleBox(
        Vector2 tileCenter,
        GameObjectCell objectCell,
        IMapObjectDestroyAccess& destroyAccess
    );

    void Update(float deltaTime) override;
    void Draw() override;
    Rectangle GetBoundingBox() const override;

    void TakeDamage(int amount);
    bool IsDestructionQueued() const { return destructionQueued; }
};
