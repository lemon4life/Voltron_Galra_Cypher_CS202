#include "Core/GameManager.h"
#include "Entities/Projectile.h"

GameManager::GameManager() : currentState(GameState::PLAYING) {
    // Starts in PLAYING state by default
}

GameManager& GameManager::GetInstance() {
    // Thread-safe in C++11+ (Meyers' Singleton)
    static GameManager instance;
    return instance;
}

void GameManager::AddProjectile(Projectile* p) {
    activeProjectiles.push_back(p);
}

void GameManager::UpdateProjectiles(float deltaTime) {
    for (auto it = activeProjectiles.begin(); it != activeProjectiles.end();) {
        (*it)->Update(deltaTime);
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
