#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "Entities/Projectile.h"
#include "Core/State/IGameState.h"
#include "Entities/GameObject.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/ParticleManager.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/TeamManager.h"
#include "raymath.h"
#include "Core/Constants.h"

GameManager::GameManager()
    : currentState(GameState::MAIN_MENU),
      previousGameState(GameState::MAIN_MENU),
      bulletImpactTex{},
      targetFPS(0),
      hitstopTimer(0.0f),
      currentFloor(1),
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

GameState GameManager::GetState() const {
    return currentState;
}

void GameManager::SetState(GameState newState) {
    currentState = newState;
}

void GameManager::SetCurrentStateObj(std::unique_ptr<IGameState> state) {
    currentStateObj = std::move(state);
}

IGameState* GameManager::GetCurrentStateObj() const {
    return currentStateObj.get();
}

std::unique_ptr<IGameState> GameManager::TakeCurrentStateObj() {
    return std::move(currentStateObj);
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

namespace {
    void DrawObjectCollisionDebug(const GameObject& object) {
        Color hitboxColor = PURPLE;
        Color collisionColor = GOLD;
        if (object.GetObjectType() == GameObjectType::Player) {
            hitboxColor = BLUE;
            collisionColor = GREEN;
        } else if (object.GetObjectType() == GameObjectType::Enemy) {
            hitboxColor = RED;
            collisionColor = ORANGE;
        }

        // A thicker hitbox outline remains visible when an entity uses the
        // same rectangle for both combat and movement collision.
        DrawRectangleLinesEx(
            object.GetBoundingBox(),
            Constants::DEBUG_COLLISION_LINE_THICKNESS * 2.0f,
            hitboxColor
        );
        DrawRectangleLinesEx(
            object.GetCollisionBox(),
            Constants::DEBUG_COLLISION_LINE_THICKNESS,
            collisionColor
        );
    }
}

void GameManager::DrawDebugOverlays(TeamManager* teamManager) const {
    if (Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES) {
        if (teamManager) {
            for (const Paladin* paladin : teamManager->GetTeam()) {
                if (paladin) {
                    DrawObjectCollisionDebug(*paladin);
                }
            }
        }

        for (const GameObject* entity : GetLevelEntities()) {
            if (entity) {
                DrawObjectCollisionDebug(*entity);
            }
        }
        for (const Projectile* projectile : activeProjectiles) {
            if (projectile && projectile->IsActive()) {
                DrawObjectCollisionDebug(*projectile);
            }
        }
        for (const std::unique_ptr<Rover>& rover : activeRovers) {
            if (rover && !rover->IsDead()) {
                DrawObjectCollisionDebug(*rover);
            }
        }
    }

    if (Constants::DEBUG_DRAW_ENEMY_PATHS) {
        for (GameObject* entity : GetLevelEntities()) {
            if (entity &&
                entity->GetObjectType() == GameObjectType::Enemy) {
                static_cast<Enemy*>(entity)->DrawPathDebug();
            }
        }
    }
}

void GameManager::UpdateProjectiles(float deltaTime, TeamManager* teamManager) {
    const auto& entities = GetLevelEntities();

    for (auto it = activeProjectiles.begin(); it != activeProjectiles.end();) {
        (*it)->Update(deltaTime);
        
        bool hitWall = false;
        bool hitEntity = false;
        Rectangle pBox = (*it)->GetBoundingBox();
        bool ignoresWorldCollision = (*it)->IgnoresWorldCollision();

        if (levelManager && !ignoresWorldCollision && !(*it)->IsReturning()) {
            hitWall = levelManager->IsSolidCollision(pBox, true);
            if (hitWall && !(*it)->IsEnemyProjectile()) {
                AddImpactEffect({pBox.x + pBox.width/2.0f, pBox.y + pBox.height/2.0f});
                ParticleManager::GetInstance().SpawnImpact(
                    { pBox.x + pBox.width / 2.0f, pBox.y + pBox.height / 2.0f },
                    (*it)->GetVelocity(), YELLOW, 8
                );
            }
        }

        for (GameObject* entity : entities) {
            if (!CheckCollisionRecs(pBox, entity->GetBoundingBox())) continue;
            
            // Box
            if (!ignoresWorldCollision && entity->GetObjectType() == GameObjectType::Box) {
                Prop& box = static_cast<Prop&>(*entity);
                if (!(*it)->HasHitTarget(entity)) {
                    box.TakeDamage((*it)->GetDamage());
                    (*it)->RecordHit(entity);
                    AddImpactEffect({ pBox.x + pBox.width / 2.0f, pBox.y + pBox.height / 2.0f });
                    hitEntity = true;
                    if (!(*it)->IsPiercing()) break;
                }
            }
            
            // Enemy
            if (!(*it)->IsEnemyProjectile() && entity->GetObjectType() == GameObjectType::Enemy) {
                Enemy* e = static_cast<Enemy*>(entity);
                if (e->IsEnabled() && !(*it)->HasHitTarget(entity)) {
                    e->TakeDamage((*it)->GetDamage());
                    (*it)->RecordHit(entity);
                    
                    Vector2 kdir = Vector2Subtract(e->GetPosition(), (*it)->GetPosition());
                    if (Vector2Length(kdir) > 0.0f) kdir = Vector2Normalize(kdir);
                    else kdir = {1.0f, 0.0f};
                    e->ApplyKnockback(kdir, 350.0f);
                    
                    Vector2 impactPos = { pBox.x + pBox.width / 2.0f, pBox.y + pBox.height / 2.0f };
                    AddImpactEffect(pBox.x > e->GetBoundingBox().x ? (Vector2){pBox.x, pBox.y} : (Vector2){pBox.x + 10, pBox.y});
                    ParticleManager::GetInstance().SpawnImpact(impactPos, (*it)->GetVelocity(), SKYBLUE, 6);
                    
                    if (teamManager && teamManager->GetActivePaladin()) {
                        teamManager->GetActivePaladin()->OnHitEnemy((*it)->GetDamage());
                    }
                    hitEntity = true;
                    if (!(*it)->IsPiercing()) break;
                }
            }
        }
        
        // Player (if enemy projectile)
        if ((*it)->IsEnemyProjectile() && teamManager && teamManager->GetActivePaladin()) {
            Paladin* activePaladin = teamManager->GetActivePaladin();
            if (CheckCollisionRecs(pBox, activePaladin->GetBoundingBox()) && !(*it)->HasHitTarget(activePaladin)) {
                if (activePaladin->CanParryAttack((*it)->GetPosition())) {
                    activePaladin->TriggerParrySuccess(*it);
                    activePaladin->IncrementParryCount();
                    TriggerHitstop(0.1f);
                    AddImpactEffect({pBox.x + pBox.width/2.0f, pBox.y + pBox.height/2.0f});
                    ParticleManager::GetInstance().SpawnParrySparks({ pBox.x + pBox.width / 2.0f, pBox.y + pBox.height / 2.0f }, 16);
                } else {
                    activePaladin->TakeDamage((*it)->GetDamage());
                    if (activePaladin->IsParrying()) {
                        activePaladin->ChangeState(activePaladin->GetIdleState()); 
                    }
                }
                (*it)->RecordHit(activePaladin);
                hitEntity = true;
            }
        }

        // Rovers (if enemy projectile)
        if ((*it)->IsEnemyProjectile()) {
            for (auto& rover : activeRovers) {
                if (!rover->IsDead() && CheckCollisionRecs(pBox, rover->GetBoundingBox()) && !(*it)->HasHitTarget(rover.get())) {
                    rover->TakeDamage((*it)->GetDamage());
                    (*it)->RecordHit(rover.get());
                    AddImpactEffect({pBox.x + pBox.width/2.0f, pBox.y + pBox.height/2.0f});
                    hitEntity = true;
                    if (!(*it)->IsPiercing()) break;
                }
            }
        }

        if (hitWall || hitEntity) {
            if ((*it)->IsPiercing() && !(*it)->IsEnemyProjectile()) {
                if (hitWall) { 
                    // Only return on STATIC walls, allowing it to pass completely through boxes
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
    ClearOrbs();
    hitstopTimer = 0.0f;
}

void GameManager::AddEffect(Vector2 pos, Texture2D tex, int frames, float lifetime, bool drawBehind, Color tint) {
    ImpactEffect effect;
    effect.position = pos;
    effect.maxLifetime = lifetime;
    effect.lifetime = lifetime;
    effect.currentFrame = 0;
    effect.numFrames = frames;
    effect.texture = tex;
    effect.drawBehind = drawBehind;
    effect.tint = tint;
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

void GameManager::SpawnQuintessenceOrb(Vector2 pos) {
    QuintessenceOrb orb;
    orb.position = pos;
    orb.velocity = { (float)GetRandomValue(-100, 100), (float)GetRandomValue(-100, 100) };
    orb.isAttracted = false;
    orb.positionHistory.clear();
    activeOrbs.push_back(orb);
}

void GameManager::UpdateOrbs(float deltaTime, TeamManager* teamManager) {
    Paladin* player = teamManager ? teamManager->GetActivePaladin() : nullptr;

    for (auto it = activeOrbs.begin(); it != activeOrbs.end(); ) {
        // Friction / Deceleration of initial pop
        if (!it->isAttracted) {
            it->velocity.x -= it->velocity.x * 4.0f * deltaTime;
            it->velocity.y -= it->velocity.y * 4.0f * deltaTime;
        }

        it->position.x += it->velocity.x * deltaTime;
        it->position.y += it->velocity.y * deltaTime;
        
        it->positionHistory.push_front(it->position);
        if (it->positionHistory.size() > 12) {
            it->positionHistory.pop_back();
        }

        if (player) {
            Vector2 playerPos = player->GetPosition();
            // Check distance to player
            float dx = playerPos.x - it->position.x;
            float dy = playerPos.y - it->position.y;
            float distSq = dx * dx + dy * dy;

            // Attract distance
            if (distSq < 75.0f * 75.0f) {
                it->isAttracted = true;
            }

            if (it->isAttracted) {
                float speed = 400.0f;
                float dist = std::sqrt(distSq);
                if (dist > 0) {
                    it->velocity.x = (dx / dist) * speed;
                    it->velocity.y = (dy / dist) * speed;
                }
            }

            // Collect distance
            if (distSq < 20.0f * 20.0f) {
                teamManager->AddQuintessence(10.0f);
                AudioManager::GetInstance().PlaySoundEffect("fx_energy");
                it = activeOrbs.erase(it);
                continue;
            }
        }
        
        ++it;
    }
}

void GameManager::DrawOrbs() {
    Texture2D tex = AssetManager::GetInstance().GetTexture("Quint_Orb");
    for (const auto& orb : activeOrbs) {
        if (orb.positionHistory.size() > 1) {
            for (size_t i = 0; i < orb.positionHistory.size() - 1; ++i) {
                float progress = 1.0f - ((float)i / orb.positionHistory.size());
                float thickness = progress * 6.0f;
                Color trailColor = ColorAlpha(SKYBLUE, progress * 0.8f);
                DrawLineEx(orb.positionHistory[i], orb.positionHistory[i+1], thickness, trailColor);
            }
        }
        
        Rectangle dest = { orb.position.x, orb.position.y, (float)tex.width, (float)tex.height };
        Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
        Vector2 origin = { (float)tex.width / 2.0f, (float)tex.height / 2.0f };
        DrawTexturePro(tex, src, dest, origin, 0.0f, WHITE);
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
            
            DrawTexturePro(effect.texture, source, dest, origin, 0.0f, effect.tint);
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
