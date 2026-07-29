#pragma once
#include "raylib.h"
#include <cmath>

enum class WeaponKinematicsType {
    Melee,
    Ranged,
    Thrust
};

class WeaponKinematics {
private:
    WeaponKinematicsType type;
    Vector2 offset;
    float angleOffset;

    // For Ranged Recoil
    Vector2 recoilVelocity;

    // For Melee/Thrust
    float animationTimer;
    float animationDuration;
    bool isAnimating;

    // Melee specific
    float swingStartAngle;
    float swingTargetAngle;

    // Thrust specific
    Vector2 thrustDir;

public:
    WeaponKinematics(WeaponKinematicsType t = WeaponKinematicsType::Melee);

    void SetType(WeaponKinematicsType t) { type = t; }

    void ApplyRecoil(Vector2 aimDir, float strength = 15.0f);
    void ApplySwing(float duration = 0.2f, float arc = 120.0f, bool reverse = false);
    void ApplyThrust(Vector2 aimDir, float duration = 0.2f);

    void Update(float deltaTime);

    Vector2 GetOffset() const { return offset; }
    float GetAngleOffset() const { return angleOffset; }
};
