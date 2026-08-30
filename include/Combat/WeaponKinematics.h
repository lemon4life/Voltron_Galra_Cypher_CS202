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
    /// Creates a WeaponKinematics instance from the supplied configuration.
    WeaponKinematics(WeaponKinematicsType t = WeaponKinematicsType::Melee);

    /// Updates the stored type.
    void SetType(WeaponKinematicsType t) { type = t; }

    /// Applies recoil.
    void ApplyRecoil(Vector2 aimDir, float strength = 15.0f);
    /// Applies swing.
    void ApplySwing(float duration = 0.2f, float arc = 120.0f, bool reverse = false);
    /// Applies thrust.
    void ApplyThrust(Vector2 aimDir, float duration = 0.2f);

    /// Advances this component's state for the current frame.
    void Update(float deltaTime);

    /// Returns the current offset.
    Vector2 GetOffset() const { return offset; }
    /// Returns the current angle offset.
    float GetAngleOffset() const { return angleOffset; }
};
