#pragma once
#include "Combat/IAttackStrategy.h"
#include "Combat/WeaponKinematics.h"
#include <vector>

class GameObject;

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
    Vector2 lastPlayerPos;
    WeaponKinematics kinematics;  // if player clicks again during active combo
    int lightDamage;
    int heavyDamage;
    
    std::vector<GameObject*> objectsHit; // Track objects hit in the current swing

public:
    MeleeAttackStrategy(
        Texture2D weapon,
        Texture2D att1,
        Texture2D att2,
        int lightDamage,
        int heavyDamage
    );
    
    void Attack(Vector2 playerPos) override;
    void Update(float deltaTime) override;
    void Draw(Vector2 playerPos, bool facingLeft) override;
    
    int GetComboStep() const { return comboStep; }
    void SetDamage(int light, int heavy) override {
        lightDamage = light;
        heavyDamage = heavy;
    }
    void SetAttackSpeedScalar(float scalar) override {
        timePerFrame = 0.05f * scalar;
    }
};
