#include "Core/AimStrategy/MouseAimStrategy.h"
#include "Entities/Player/Paladin.h"
#include "raymath.h"

Vector2 MouseAimStrategy::CalculateAimVector(Paladin* paladin) {
    Vector2 aimDir = Vector2Subtract(paladin->GetAimTarget(), paladin->GetWeaponPivot());
    if (Vector2Length(aimDir) > 0.1f) {
        return Vector2Normalize(aimDir);
    }
    // Fallback if mouse is exactly on player
    float angle = paladin->GetCurrentAimAngle();
    return { cosf(angle), sinf(angle) };
}
