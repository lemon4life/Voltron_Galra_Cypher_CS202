#pragma once
#include "Entities/GameObject.h"

class Wall : public GameObject {
public:
    /// Creates a Wall instance from the supplied configuration.
    Wall(Vector2 pos);
    
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
};
