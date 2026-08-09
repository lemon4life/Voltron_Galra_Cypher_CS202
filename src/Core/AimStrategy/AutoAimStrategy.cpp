#include "Core/AimStrategy/AutoAimStrategy.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "Core/Manager/InputManager.h"
#include "raymath.h"

Vector2 AutoAimStrategy::CalculateAimVector(Paladin* paladin) {
    if (paladin->GetLockedEnemy() && !paladin->IsDoingUltimate()) {
        Vector2 aimDir = Vector2Subtract(paladin->GetLockedEnemy()->GetPosition(), paladin->GetPosition());
        if (Vector2Length(aimDir) > 0.1f) {
            return Vector2Normalize(aimDir);
        }
    }
    
    // Fallback to movement vector
    Vector2 moveDir = InputManager::GetMovementVector();
    if (Vector2Length(moveDir) > 0.1f) {
        return Vector2Normalize(moveDir);
    }
    
    // Ultimate fallback to current angle
    float angle = paladin->GetCurrentAimAngle();
    return { cosf(angle), sinf(angle) };
}
