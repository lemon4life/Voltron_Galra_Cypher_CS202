#pragma once
#include "raylib.h"

class GameObject {
protected:
    Vector2 position;
    Rectangle boundingBox;

public:
    GameObject(Vector2 pos) : position(pos), boundingBox{pos.x, pos.y, 0, 0} {}
    virtual ~GameObject() = default;

    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;

    Vector2 GetPosition() const { return position; }
    void SetPosition(Vector2 pos) { position = pos; }
};
