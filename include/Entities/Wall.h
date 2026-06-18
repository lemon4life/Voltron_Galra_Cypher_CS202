#pragma once
#include "Entities/GameObject.h"

class Wall : public GameObject {
public:
    Wall(Vector2 pos);
    
    void Update(float deltaTime) override;
    void Draw() override;
};
