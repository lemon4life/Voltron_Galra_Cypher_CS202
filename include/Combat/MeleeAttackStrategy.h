#pragma once
#include "Combat/IAttackStrategy.h"
#include "Combat/WeaponKinematics.h"
#include "Core/World/ObjectId.h"

#include <unordered_set>
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
    float lastCollisionAngleOffset;
    bool lastFacingLeft;
    
    std::unordered_set<ObjectId> objectsHit;
    std::unordered_set<MapObjectHandle> mapObjectsHit;

    /// Returns the current signed swing offset.
    float GetSignedSwingOffset() const;
    /// Processes blade collision.
    void ProcessBladeCollision(
        float startAngleDegrees,
        float endAngleDegrees
    );

public:
    /// Creates a MeleeAttackStrategy instance from the supplied configuration.
    MeleeAttackStrategy(
        Texture2D weapon,
        Texture2D att1,
        Texture2D att2,
        int lightDamage,
        int heavyDamage
    );
    
    /// Starts this attack behavior when its current conditions allow it.
    void Attack(Vector2 playerPos) override;
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw(Vector2 playerPos, bool facingLeft) override;
    
    /// Returns the current combo step.
    int GetComboStep() const { return comboStep; }
    /// Updates the stored damage.
    void SetDamage(int light, int heavy) override {
        lightDamage = light;
        heavyDamage = heavy;
    }
    /// Updates the stored attack speed scalar.
    void SetAttackSpeedScalar(float scalar) override {
        timePerFrame = 0.05f * scalar;
    }
};
