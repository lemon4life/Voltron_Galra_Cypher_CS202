#pragma once
#include "Combat/IAttackStrategy.h"
#include <vector>

class GameObject;
class Enemy;

class MeleeAttackStrategy : public IAttackStrategy {
private:
    Texture2D weaponTex;
    Texture2D attack1Tex;
    Texture2D attack2Tex;
    
    int comboStep;       // 0: none, 1: attack1, 2: attack2
    int nextComboStep;   // toggles between 1 and 2
    float frameTimer;    // time accumulated for current frame
    int currentFrame;    // 0 to 3
    float timePerFrame;  // attack speed scaling
    bool inputBuffered;
    Vector2 lastPlayerPos;  // if player clicks again during active combo
    
    std::vector<Enemy*> enemiesHit; // Track who got hit in the current swing

public:
    MeleeAttackStrategy(Texture2D weapon, Texture2D att1, Texture2D att2);
    
    void Attack(Vector2 playerPos) override;
    void Update(float deltaTime) override;
    void Draw(Vector2 playerPos, bool facingLeft) override;
    
    int GetComboStep() const { return comboStep; }
};
