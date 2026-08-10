#include "Core/Manager/GameManager.h"
#include "Entities/Projectile.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/ParticleManager.h"
#include "raymath.h"

GameManager::GameManager()
    : currentState(GameState::MAIN_MENU),
      previousGameState(GameState::MAIN_MENU),
      bulletImpactTex{},
      targetFPS(0),
      hitstopTimer(0.0f),
      levelManager(nullptr) {
    // Starts in MAIN_MENU state by default
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

bool GameManager::PauseGame() {
    if (currentState != GameState::HUB &&
        currentState != GameState::GAMEPLAY) {
        return false;
    }

    previousGameState = currentState;
    currentState = GameState::PAUSE;
    return true;
}

bool GameManager::ResumeGame() {
    if (currentState != GameState::PAUSE) {
        return false;
    }

    currentState = previousGameState;
    return true;
}

bool GameManager::IsPaused() const {
    return currentState == GameState::PAUSE;
}

GameState GameManager::GetPreviousGameState() const {
    return previousGameState;
}

GameState GameManager::GetRenderState() const {
    return IsPaused() ? previousGameState : currentState;
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
#include "Entities/Rover.h"
#include "Entities/Props/Prop.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"

void GameManager::UpdateProjectiles(float deltaTime, TeamManager* teamManager) {
    const auto& entities = GetLevelEntities();

    for (auto it = activeProjectiles.begin(); it != activeProjectiles.end();) {
        (*it)->Update(deltaTime);
        
        bool hitSomething = false;
        Rectangle pBox = (*it)->GetBoundingBox();

        // Damage box entities before the solid object-grid cell consumes the projectile.
        for (GameObject* entity : entities) {
            if (entity->GetObjectType() != GameObjectType::Box) {
                continue;
            }
            if (!CheckCollisionRecs(pBox, entity->GetBoundingBox())) {
                continue;
            }

            Prop& box = static_cast<Prop&>(*entity);
            box.TakeDamage((*it)->GetDamage());
            AddImpactEffect({
                pBox.x + pBox.width / 2.0f,
                pBox.y + pBox.height / 2.0f
            });
            hitSomething = true;
            break;
        }

        // We no longer skip all collision checks here. Returning weapons must hit enemies and boxes.
        // We will explicitly skip SOLID environment collisions further down.

        // Check collision with Player (if it's an enemy projectile)
        if (!hitSomething && (*it)->IsEnemyProjectile() && teamManager && teamManager->GetActivePaladin()) {
            Paladin* activePaladin = teamManager->GetActivePaladin();
            if (CheckCollisionRecs(pBox, activePaladin->GetBoundingBox())) {
                if (activePaladin->CanParryAttack((*it)->GetPosition())) {
                    activePaladin->TriggerParrySuccess(*it);
                    activePaladin->IncrementParryCount();
                    TriggerHitstop(0.1f);
                    AddImpactEffect({pBox.x + pBox.width/2.0f, pBox.y + pBox.height/2.0f});
                    // Parry sparks — burst at the point of contact
                    ParticleManager::GetInstance().SpawnParrySparks(
                        { pBox.x + pBox.width / 2.0f, pBox.y + pBox.height / 2.0f }, 16
                    );
                } else {
                    activePaladin->TakeDamage((*it)->GetDamage());
                    if (activePaladin->IsParrying()) {
                        activePaladin->ChangeState(activePaladin->GetIdleState()); // Break parry on failed block
                    }
                }
                hitSomething = true;
            }
        }
        
        // If it's an enemy projectile, check collision with Rovers
        if (!hitSomething && (*it)->IsEnemyProjectile()) {
            for (auto& rover : activeRovers) {
                if (!rover->IsDead() && CheckCollisionRecs(pBox, rover->GetBoundingBox())) {
                    rover->TakeDamage((*it)->GetDamage());
                    AddImpactEffect({pBox.x + pBox.width/2.0f, pBox.y + pBox.height/2.0f});
                    hitSomething = true;
                    break;
                }
            }
        }

        // Check collision with environment and enemies (if it's a player projectile)
        if (!hitSomething) {
            // First check level manager for static walls, ONLY if it's not returning
            if (levelManager && !(*it)->IsReturning() && levelManager->IsSolidCollision(pBox)) {
                hitSomething = true;
                if (!(*it)->IsEnemyProjectile()) {
                    AddImpactEffect({pBox.x + pBox.width/2.0f, pBox.y + pBox.height/2.0f});
                    // Wall impact debris — splatter in the direction the projectile came from
                    ParticleManager::GetInstance().SpawnImpact(
                        { pBox.x + pBox.width / 2.0f, pBox.y + pBox.height / 2.0f },
                        (*it)->GetVelocity(), YELLOW, 8
                    );
                }
            } else {
                for (auto* entity : entities) {
                    if (CheckCollisionRecs(pBox, entity->GetBoundingBox())) {
                        if (entity->GetObjectType() == GameObjectType::Enemy) {
                            Enemy* e = static_cast<Enemy*>(entity);
                            if (!(*it)->IsEnemyProjectile()) {
                                e->TakeDamage((*it)->GetDamage());
                                Vector2 kdir = Vector2Subtract(e->GetPosition(), (*it)->GetPosition());
                                if (Vector2Length(kdir) > 0.0f) kdir = Vector2Normalize(kdir);
                                else kdir = {1.0f, 0.0f};
                                e->ApplyKnockback(kdir, 350.0f);
                                Vector2 impactPos = { pBox.x + pBox.width / 2.0f, pBox.y + pBox.height / 2.0f };
                                AddImpactEffect(pBox.x > e->GetBoundingBox().x ? (Vector2){pBox.x, pBox.y} : (Vector2){pBox.x + 10, pBox.y});
                                // Enemy hit debris — cyan tinted splatter
                                ParticleManager::GetInstance().SpawnImpact(
                                    impactPos, (*it)->GetVelocity(), SKYBLUE, 6
                                );
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
            if ((*it)->IsPiercing() && !(*it)->IsEnemyProjectile()) {
                if (levelManager && levelManager->IsSolidCollision(pBox)) {
                    (*it)->SetReturning(true);
                }
            } else {
                (*it)->Destroy();
            }
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
    for (const auto p : activeProjectiles) {
        if (p->GetOwner() != nullptr && p->IsReturning()) {
            DrawLineEx(p->GetPosition(), p->GetOwner()->GetPosition(), 2.0f, GREEN);
        }
        p->Draw();
    }
}

void GameManager::AddDepthRenderItems(std::vector<DepthRenderItem>& items) {
    for (auto* p : activeProjectiles) {
        items.push_back({
            p->GetBoundingBox().y + p->GetBoundingBox().height,
            [p]() { p->Draw(); }
        });
    }
    for (auto& r : activeRovers) {
        items.push_back({ r->GetPosition().y, [&r]() { r->Draw(); } });
    }
}

void GameManager::ClearProjectiles() {
    for (auto p : activeProjectiles) {
        delete p;
    }
    activeProjectiles.clear();
}

void GameManager::ResetTransientState() {
    ClearProjectiles();
    activeEffects.clear();
    hitstopTimer = 0.0f;
}

void GameManager::AddEffect(Vector2 pos, Texture2D tex, int frames, float lifetime, bool drawBehind) {
    ImpactEffect effect;
    effect.position = pos;
    effect.maxLifetime = lifetime;
    effect.lifetime = lifetime;
    effect.currentFrame = 0;
    effect.numFrames = frames;
    effect.texture = tex;
    effect.drawBehind = drawBehind;
    activeEffects.push_back(effect);
}

void GameManager::AddImpactEffect(Vector2 pos) {
    AddEffect(pos, bulletImpactTex, 4, 0.2f);
}

void GameManager::UpdateEffects(float deltaTime) {
    for (auto it = activeEffects.begin(); it != activeEffects.end();) {
        it->lifetime -= deltaTime;
        if (it->lifetime <= 0.0f) {
            it = activeEffects.erase(it);
        } else {
            if (it->texture.id != 0) {
                float progress = 1.0f - (it->lifetime / it->maxLifetime);
                it->currentFrame = (int)(progress * it->numFrames);
                if (it->currentFrame >= it->numFrames) it->currentFrame = it->numFrames - 1;
            }
            ++it;
        }
    }
}

void GameManager::DrawEffects(bool background) {
    for (const auto& effect : activeEffects) {
        if (effect.drawBehind == background && effect.texture.id != 0) {
            float frameWidth = (float)effect.texture.width / effect.numFrames;
            float frameHeight = (float)effect.texture.height;
            Rectangle source = { effect.currentFrame * frameWidth, 0.0f, frameWidth, frameHeight };
            Rectangle dest = { effect.position.x, effect.position.y, frameWidth, frameHeight };
            Vector2 origin = { frameWidth / 2.0f, frameHeight / 2.0f };
            
            DrawTexturePro(effect.texture, source, dest, origin, 0.0f, WHITE);
        }
    }
}

void GameManager::AddRover(std::unique_ptr<Rover> rover) {
    if (!activeRovers.empty()) {
        activeRovers.front()->Heal();
    } else {
        activeRovers.push_back(std::move(rover));
    }
}

// Removed AddVenomZone

void GameManager::UpdateAssists(float deltaTime, TeamManager* teamManager) {
    // Update Rovers
    for (auto& rover : activeRovers) {
        rover->Update(deltaTime);
    }
    
    activeRovers.erase(std::remove_if(activeRovers.begin(), activeRovers.end(),
        [](const std::unique_ptr<Rover>& r) { return r->IsDead(); }), activeRovers.end());
}
