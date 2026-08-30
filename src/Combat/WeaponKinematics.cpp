#include "Combat/WeaponKinematics.h"

/// Creates a WeaponKinematics instance from the supplied configuration.
WeaponKinematics::WeaponKinematics(WeaponKinematicsType t)
    : type(t), offset{0.0f, 0.0f}, angleOffset(0.0f),
      recoilVelocity{0.0f, 0.0f}, animationTimer(0.0f),
      animationDuration(0.0f), isAnimating(false),
      swingStartAngle(0.0f), swingTargetAngle(0.0f),
      thrustDir{0.0f, 0.0f}
{}

void WeaponKinematics::ApplyRecoil(Vector2 aimDir, float strength) {
    if (type != WeaponKinematicsType::Ranged) return;
    recoilVelocity = { -aimDir.x * strength, -aimDir.y * strength };
    offset = recoilVelocity;
}

/// Applies swing.
void WeaponKinematics::ApplySwing(float duration, float arc, bool reverse) {
    if (type != WeaponKinematicsType::Melee) return;
    isAnimating = true;
    animationTimer = 0.0f;
    animationDuration = duration;
    if (reverse) {
        swingStartAngle = arc / 2.0f;
        swingTargetAngle = -arc / 2.0f;
    } else {
        swingStartAngle = -arc / 2.0f;
        swingTargetAngle = arc / 2.0f;
    }
    angleOffset = swingStartAngle;
}

/// Applies thrust.
void WeaponKinematics::ApplyThrust(Vector2 aimDir, float duration) {
    if (type != WeaponKinematicsType::Thrust) return;
    isAnimating = true;
    animationTimer = 0.0f;
    animationDuration = duration;
    thrustDir = aimDir;
    offset = {0.0f, 0.0f};
}

/// Advances this component's state for the current frame.
void WeaponKinematics::Update(float deltaTime) {
    if (type == WeaponKinematicsType::Ranged) {
        // Exponential decay for recoil
        recoilVelocity.x -= recoilVelocity.x * 15.0f * deltaTime;
        recoilVelocity.y -= recoilVelocity.y * 15.0f * deltaTime;
        offset = recoilVelocity;
    } 
    else if (isAnimating) {
        animationTimer += deltaTime;
        float t = animationTimer / animationDuration;
        if (t >= 1.0f) {
            t = 1.0f;
            isAnimating = false;
        }

        if (type == WeaponKinematicsType::Melee) {
            // Sine-wave interpolation for smooth swing (ease in out)
            float easedT = (1.0f - cosf(t * PI)) / 2.0f;
            angleOffset = swingStartAngle + (swingTargetAngle - swingStartAngle) * easedT;
            if (!isAnimating) {
                angleOffset = 0.0f; // Reset to center
            }
        } 
        else if (type == WeaponKinematicsType::Thrust) {
            // Thrust goes out and comes back
            float distance = 16.0f; // Max thrust distance
            float easedT;
            if (t < 0.5f) {
                easedT = t * 2.0f; // Ease out
            } else {
                easedT = 2.0f - (t * 2.0f); // Ease back in
            }
            offset = { thrustDir.x * distance * easedT, thrustDir.y * distance * easedT };
        }
    }
}
