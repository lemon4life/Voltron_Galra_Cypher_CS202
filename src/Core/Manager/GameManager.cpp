#include "Core/Manager/GameManager.h"
#include "Entities/Projectile.h"
#include "Core/Manager/LevelManager.h"

GameManager::GameManager() : currentState(GameState::MENU), levelManager(nullptr) {
    // Starts in MENU state by default
}

GameManager::~GameManager() {
    // Note: levelManager is owned by main.cpp in this design, so we don't delete it here.
    for (auto p : activeProjectiles) {
        delete p;
    }
    activeProjectiles.clear();
}

GameManager& GameManager::GetInstance() {
    // Thread-safe in C++11+ (Meyers' Singleton)
    static GameManager instance;
    return instance;
}

const std::vector<GameObject*>& GameManager::GetLevelEntities() const {
    static std::vector<GameObject*> emptyList;
    if (levelManager) {
        return levelManager->GetEntities();
    }
    return emptyList;
}

void GameManager::AddProjectile(Projectile* p) {
    activeProjectiles.push_back(p);
}

#include "Entities/Enemy.h"
#include "Entities/Player/Player.h"

void GameManager::UpdateProjectiles(float deltaTime, Player* player) {
    const auto& entities = GetLevelEntities();

    for (auto it = activeProjectiles.begin(); it != activeProjectiles.end();) {
        (*it)->Update(deltaTime);
        
        bool hitSomething = false;
        Rectangle pBox = (*it)->GetBoundingBox();

        // If it's an enemy projectile, check collision with Player
        if ((*it)->IsEnemyProjectile() && player) {
            if (CheckCollisionRecs(pBox, player->GetBoundingBox())) {
                player->TakeDamage((*it)->GetDamage());
                hitSomething = true;
            }
        }

        // Check collision with environment and enemies (if it's a player projectile)
        if (!hitSomething) {
            // First check level manager for static walls
            if (levelManager && levelManager->IsSolidCollision(pBox)) {
                hitSomething = true;
            } else {
                for (auto* entity : entities) {
                    if (CheckCollisionRecs(pBox, entity->GetBoundingBox())) {
                        if (Enemy* e = dynamic_cast<Enemy*>(entity)) {
                            if (!(*it)->IsEnemyProjectile()) {
                                e->TakeDamage((*it)->GetDamage());
                                hitSomething = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (hitSomething) {
            (*it)->Destroy();
        }

        if (!(*it)->IsActive()) {
            delete *it;
            it = activeProjectiles.erase(it);
        } else {
            ++it;
        }
    }
}

void GameManager::DrawProjectiles() {
    for (auto p : activeProjectiles) {
        p->Draw();
    }
}

void GameManager::ClearProjectiles() {
    for (auto p : activeProjectiles) {
        delete p;
    }
    activeProjectiles.clear();
}
