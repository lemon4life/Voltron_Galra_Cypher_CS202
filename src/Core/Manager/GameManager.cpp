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
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"

void GameManager::UpdateProjectiles(float deltaTime, TeamManager* teamManager) {
    const auto& entities = GetLevelEntities();

    for (auto it = activeProjectiles.begin(); it != activeProjectiles.end();) {
        (*it)->Update(deltaTime);
        
        bool hitSomething = false;
        Rectangle pBox = (*it)->GetBoundingBox();

        // If it's an enemy projectile, check collision with Player
        if ((*it)->IsEnemyProjectile() && teamManager && teamManager->GetActivePaladin()) {
            Paladin* activePaladin = teamManager->GetActivePaladin();
            if (CheckCollisionRecs(pBox, activePaladin->GetBoundingBox())) {
                if (activePaladin->CanParryAttack((*it)->GetPosition())) {
                    activePaladin->TriggerParrySuccess(*it);
                    activePaladin->IncrementParryCount();
                    TriggerHitstop(0.1f);
                    AddImpactEffect({pBox.x + pBox.width/2.0f, pBox.y + pBox.height/2.0f});
                } else {
                    activePaladin->TakeDamage((*it)->GetDamage());
                    if (activePaladin->IsParrying()) {
                        activePaladin->ChangeState(activePaladin->GetIdleState()); // Break parry on failed block
                    }
                }
                hitSomething = true;
            }
        }

        // Check collision with environment and enemies (if it's a player projectile)
        if (!hitSomething) {
            // First check level manager for static walls
            if (levelManager && levelManager->IsSolidCollision(pBox)) {
                hitSomething = true;
                if (!(*it)->IsEnemyProjectile()) {
                    AddImpactEffect({pBox.x + pBox.width/2.0f, pBox.y + pBox.height/2.0f});
                }
            } else {
                for (auto* entity : entities) {
                    if (CheckCollisionRecs(pBox, entity->GetBoundingBox())) {
                        if (Enemy* e = dynamic_cast<Enemy*>(entity)) {
                            if (!(*it)->IsEnemyProjectile()) {
                                e->TakeDamage((*it)->GetDamage());
                                AddImpactEffect(pBox.x > e->GetBoundingBox().x ? (Vector2){pBox.x, pBox.y} : (Vector2){pBox.x + 10, pBox.y});
                                if (teamManager && teamManager->GetActivePaladin()) {
                                    teamManager->GetActivePaladin()->OnHitEnemy((*it)->GetDamage());
                                }
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

void GameManager::AddEffect(Vector2 pos, Texture2D tex, int frames, float lifetime) {
    ImpactEffect effect;
    effect.position = pos;
    effect.maxLifetime = lifetime;
    effect.lifetime = lifetime;
    effect.currentFrame = 0;
    effect.numFrames = frames;
    effect.texture = tex;
    activeEffects.push_back(effect);
}

void GameManager::AddImpactEffect(Vector2 pos) {
    AddEffect(pos, bulletImpactTex, 4, 0.2f);
}

void GameManager::UpdateAndDrawEffects(float deltaTime) {
    for (auto it = activeEffects.begin(); it != activeEffects.end();) {
        it->lifetime -= deltaTime;
        
        if (it->texture.id != 0) {
            float progress = 1.0f - (it->lifetime / it->maxLifetime);
            it->currentFrame = (int)(progress * it->numFrames);
            if (it->currentFrame >= it->numFrames) it->currentFrame = it->numFrames - 1;
            
            float frameWidth = (float)it->texture.width / it->numFrames;
            float frameHeight = (float)it->texture.height;
            Rectangle source = { it->currentFrame * frameWidth, 0.0f, frameWidth, frameHeight };
            Rectangle dest = { it->position.x, it->position.y, frameWidth, frameHeight };
            Vector2 origin = { frameWidth / 2.0f, frameHeight / 2.0f };
            
            DrawTexturePro(it->texture, source, dest, origin, 0.0f, WHITE);
        }
        
        if (it->lifetime <= 0.0f) {
            it = activeEffects.erase(it);
        } else {
            ++it;
        }
    }
}
