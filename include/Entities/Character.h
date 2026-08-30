#pragma once
#include "GameObject.h"

class Character : public GameObject {
protected:
    float speed;
    int health;
    Texture2D texture; // The currently active texture

public:
    /// Creates a Character instance from the supplied configuration.
    Character(Vector2 pos, float spd, int hp, Texture2D tex)
        : GameObject(pos, GameObjectType::Player),
          speed(spd),
          health(hp),
          texture(tex) {}
    
    /// Releases resources owned by this Character instance.
    virtual ~Character() = default;

    /// Returns the current speed.
    float GetSpeed() const { return speed; }
    /// Updates the stored speed.
    void SetSpeed(float spd) { speed = spd; }
    
    /// Returns the current health.
    int GetHealth() const { return health; }
    /// Updates the stored health.
    void SetHealth(int hp) { health = hp; }

    /// Returns the current texture.
    Texture2D GetTexture() const { return texture; }
    /// Updates the stored texture.
    void SetTexture(Texture2D tex) { texture = tex; }
};
