#include "AI/DroneState.h"
#include "Entities/EnemyEntities/Drone.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "raymath.h"
#include <cstdlib>

DroneState::DroneState() : hoverTimer(0.0f) {}

void DroneState::Enter(Enemy* enemy) {
    Drone* drone = static_cast<Drone*>(enemy);
    hoverTimer = 0.0f;
    FindNewHoverTarget(drone);
}

void DroneState::Exit(Enemy* enemy) {
    enemy->SetCurrentVelocity({0.0f, 0.0f});
}

bool DroneState::FindNewHoverTarget(Drone* drone) {
    TeamManager* targetTeam = drone->GetTargetTeam();
    if (!targetTeam) return false;
    
    Paladin* target = targetTeam->GetActivePaladin();
    if (!target) return false;
    
    Vector2 playerPos = target->GetPosition();
    
    // Pick a random spot around the player, roughly 200 units away
    float angle = (rand() % 360) * DEG2RAD;
    float dist = 150.0f + (rand() % 100);
    
    hoverTarget = { playerPos.x + cosf(angle) * dist, playerPos.y + sinf(angle) * dist };
    
    return true;
}

void DroneState::Update(Enemy* enemy, float deltaTime) {
    Drone* drone = static_cast<Drone*>(enemy);
    drone->DecreaseCooldown(deltaTime);
    
    TeamManager* targetTeam = drone->GetTargetTeam();
    if (!targetTeam) return;
    
    Paladin* target = targetTeam->GetActivePaladin();
    if (!target) return;
    
    Vector2 playerPos = target->GetPosition();
    
    // Face player
    drone->SetFacingLeft(playerPos.x < drone->GetPosition().x);
    
    // Move towards hover target
    Vector2 toTarget = Vector2Subtract(hoverTarget, drone->GetPosition());
    float dist = Vector2Length(toTarget);
    
    if (dist > 10.0f) {
        Vector2 dir = Vector2Normalize(toTarget);
        drone->SetCurrentVelocity(Vector2Scale(dir, drone->GetSpeed()));
    } else {
        drone->SetCurrentVelocity({0.0f, 0.0f});
        hoverTimer -= deltaTime;
        if (hoverTimer <= 0.0f) {
            FindNewHoverTarget(drone);
            hoverTimer = 1.0f + (rand() % 200) / 100.0f; // 1 to 3 seconds
        }
    }
    
    // Move the drone with collision
    Vector2 vel = drone->GetCurrentVelocity();
    drone->UpdateMovement(vel, deltaTime, EnemyWallResponse::Slide);
    
    // Attack
    if (drone->GetAttackCooldown() <= 0.0f) {
        // Must have line of sight to attack
        if (drone->GetLineOfSightQuery().HasClearLineOfSight(drone->GetPosition(), playerPos, 5.0f)) {
            drone->Attack();
        } else {
            // Can't see player, find new spot immediately
            FindNewHoverTarget(drone);
            drone->ResetAttackCooldown(); // maybe reduce cooldown a bit instead of full reset
        }
    }
}
