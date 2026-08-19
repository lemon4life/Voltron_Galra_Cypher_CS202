#include "Core/AimStrategy/AutoAimStrategy.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/InputManager.h"
#include "raymath.h"

Vector2 AutoAimStrategy::CalculateAimVector(Paladin* paladin) {
    float bestDist = 400.0f; // Targeting range
    Enemy* bestTarget = nullptr;
    Vector2 playerPos = paladin->GetPosition();
    
    for (auto* entity : GameManager::GetInstance().GetLevelEntities()) {
        if (entity->GetObjectType() == GameObjectType::Enemy) {
            Enemy* enemy = static_cast<Enemy*>(entity);
            if (enemy->IsDead() || !enemy->IsEnabled()) continue;
            
            Vector2 toEnemy = {enemy->GetPosition().x - playerPos.x, enemy->GetPosition().y - playerPos.y};
            float dist = Vector2Length(toEnemy);
            
            if (dist < bestDist) {
                bestDist = dist;
                bestTarget = enemy;
            }
        }
    }
    
    paladin->SetLockedEnemy(bestTarget);
    
    if (bestTarget && !paladin->IsDoingUltimate()) {
        Vector2 aimDir = Vector2Subtract(bestTarget->GetPosition(), paladin->GetWeaponPivot());
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
