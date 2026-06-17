#pragma once
#include "GameObject.h"

class Character : public GameObject {
protected:
    float speed;
    int health;
    Texture2D texture; // The currently active texture

public:
    Character(Vector2 pos, float spd, int hp, Texture2D tex)
        : GameObject(pos), speed(spd), health(hp), texture(tex) {}
    
    virtual ~Character() = default;

    float GetSpeed() const { return speed; }
    void SetSpeed(float spd) { speed = spd; }
    
    int GetHealth() const { return health; }
    void SetHealth(int hp) { health = hp; }

    Texture2D GetTexture() const { return texture; }
    void SetTexture(Texture2D tex) { texture = tex; }
};
