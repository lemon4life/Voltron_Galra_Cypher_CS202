#include "Core/Manager/ObjectManager.h"

#include "Core/Constants.h"
#include "Core/EntityFactory.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/EffectManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Enemy.h"
#include "Entities/GameObject.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Projectile.h"
#include "Entities/Props/Pot.h"
#include "Entities/Rover.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <string>

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

ObjectManager::ObjectManager() = default;

ObjectManager::~ObjectManager() {
    Clear();
}

void ObjectManager::Configure(
    LevelManager& level,
    IEnemyPathAccess& paths,
    EffectManager& effects,
    TeamManager* team
) {
    levelManager = &level;
    pathFinding = &paths;
    effectManager = &effects;
    teamManager = team;
}

void ObjectManager::RebuildViews() {
    enemyView.clear();
    interactableView.clear();
    enemyView.reserve(enemies.size());
    interactableView.reserve(interactables.size());

    for (const std::unique_ptr<Enemy>& enemy : enemies) {
        if (!enemy) continue;
        enemyView.push_back(enemy.get());
    }
    for (const std::unique_ptr<GameObject>& interactable : interactables) {
        if (!interactable) continue;
        interactableView.push_back(interactable.get());
    }
}

void ObjectManager::RouteObject(std::unique_ptr<GameObject> object) {
    if (!object) return;

    if (auto* enemy = dynamic_cast<Enemy*>(object.get())) {
        object.release();
        enemies.emplace_back(enemy);
        return;
    }
    if (auto* pickup = dynamic_cast<Pot*>(object.get())) {
        object.release();
        pickups.emplace_back(pickup);
        return;
    }
    interactables.push_back(std::move(object));
}

void ObjectManager::AddObject(std::unique_ptr<GameObject> object) {
    RouteObject(std::move(object));
    RebuildViews();
}

void ObjectManager::SpawnAll(const DynamicSpawnList& requests) {
    for (const DynamicSpawnRequest& request : requests) {
        Spawn(request.type, request.position);
    }
}

GameObject* ObjectManager::Spawn(MapObjectId type, Vector2 position) {
    if (!teamManager || !pathFinding || !levelManager) return nullptr;
    std::unique_ptr<GameObject> object = EntityFactory::CreateEntity(
        type,
        position,
        teamManager,
        *this,
        *pathFinding,
        *levelManager
    );
    if (!object) return nullptr;
    GameObject* result = object.get();
    RouteObject(std::move(object));
    RebuildViews();
    return result;
}

bool ObjectManager::QueueSpawn(MapObjectId type, Vector2 position) {
    if (!teamManager || !pathFinding || !levelManager) return false;

    std::unique_ptr<GameObject> object = EntityFactory::CreateEntity(
        type,
        position,
        teamManager,
        *this,
        *pathFinding,
        *levelManager
    );
    if (!object) return false;
    pendingAddition.push_back(std::move(object));
    return true;
}

void ObjectManager::ProcessPendingAdditions() {
    for (std::unique_ptr<GameObject>& object : pendingAddition) {
        RouteObject(std::move(object));
    }
    pendingAddition.clear();
    RebuildViews();
}

void ObjectManager::QueueRemoval(GameObject* object) {
    if (object) pendingRemoval.insert(object);
}

void ObjectManager::DeleteAllEnemies() {
    for (const std::unique_ptr<Enemy>& enemy : enemies) {
        if (!enemy) continue;
        silentRemoval.insert(enemy->GetObjectId());
        pendingRemoval.insert(enemy.get());
    }
}

void ObjectManager::FinalizeEnemyDeath(Enemy& enemy) {
    ObjectId id = enemy.GetObjectId();
    if (finalizedDeaths.count(id) || silentRemoval.count(id)) return;
    finalizedDeaths.insert(id);

    Texture2D downTexture =
        AssetManager::GetInstance().GetTexture("Enemy_Down");
    if (enemy.GetEnemyType() == EnemyType::DRONE) {
        downTexture = AssetManager::GetInstance().GetTexture("Drone_down");
    } else if (enemy.GetEnemyType() == EnemyType::DEMON_THA) {
        downTexture = AssetManager::GetInstance().GetTexture("THA_Down");
    }
    effectManager->AddCorpse(
        enemy.GetPosition(),
        downTexture,
        enemy.IsFacingLeft(),
        enemy.GetKnockbackVelocity()
    );
    SpawnQuintessenceOrb(enemy.GetPosition());

    int soundIndex = GetRandomValue(0, 1);
    const char* soundPrefix = enemy.GetEnemyType() == EnemyType::DRONE
        ? "drone_dead_"
        : "knight_dead_";
    AudioManager::GetInstance().PlaySoundEffectVolume(
        std::string(soundPrefix) + std::to_string(soundIndex),
        0.25f
    );
}

void ObjectManager::ProcessPendingRemovals() {
    if (pendingRemoval.empty()) return;

    enemies.erase(
        std::remove_if(
            enemies.begin(),
            enemies.end(),
            [this](const std::unique_ptr<Enemy>& enemy) {
                if (!enemy || !pendingRemoval.count(enemy.get())) return false;
                if (enemy->IsDead()) FinalizeEnemyDeath(*enemy);
                finalizedDeaths.erase(enemy->GetObjectId());
                silentRemoval.erase(enemy->GetObjectId());
                return true;
            }
        ),
        enemies.end()
    );
    pickups.erase(
        std::remove_if(
            pickups.begin(),
            pickups.end(),
            [this](const std::unique_ptr<Pot>& pickup) {
                return pickup && pendingRemoval.count(pickup.get());
            }
        ),
        pickups.end()
    );
    interactables.erase(
        std::remove_if(
            interactables.begin(),
            interactables.end(),
            [this](const std::unique_ptr<GameObject>& object) {
                return object && pendingRemoval.count(object.get());
            }
        ),
        interactables.end()
    );
    pendingRemoval.clear();
    RebuildViews();
}

void ObjectManager::UpdateEntities(float deltaTime) {
    for (const std::unique_ptr<Enemy>& enemy : enemies) {
        if (!enemy) continue;
        if (enemy->IsDead()) {
            QueueRemoval(enemy.get());
            continue;
        }
        enemy->Update(deltaTime);
        if (enemy->IsDead()) QueueRemoval(enemy.get());
    }

    for (const std::unique_ptr<Pot>& pickup : pickups) {
        if (!pickup) continue;
        pickup->Update(deltaTime);
        if (pickup->IsConsumed()) QueueRemoval(pickup.get());
    }
    for (const std::unique_ptr<GameObject>& interactable : interactables) {
        if (interactable) interactable->Update(deltaTime);
    }

    ProcessPendingAdditions();
}

void ObjectManager::AddProjectile(
    std::unique_ptr<Projectile> projectile
) {
    if (projectile) projectiles.push_back(std::move(projectile));
}

void ObjectManager::AddProjectile(Projectile* projectile) {
    AddProjectile(std::unique_ptr<Projectile>(projectile));
}

void ObjectManager::UpdateProjectiles(float deltaTime) {
    if (!levelManager) return;

    for (auto iterator = projectiles.begin(); iterator != projectiles.end();) {
        Projectile& projectile = **iterator;
        projectile.Update(deltaTime);
        if (!projectile.IsActive()) {
            iterator = projectiles.erase(iterator);
            continue;
        }

        Rectangle projectileBox = projectile.GetBoundingBox();
        bool ignoresWorld = projectile.IgnoresWorldCollision();
        MapObject* hitMapObject = nullptr;
        bool hitWall = false;
        bool hitEntity = false;

        if (!ignoresWorld && !projectile.IsReturning()) {
            hitMapObject = levelManager->FindSolidMapObjectCollision(
                projectileBox
            );
            hitWall = levelManager->IsSolidCollision(projectileBox);

            if (hitMapObject &&
                !projectile.HasHitMapObject(hitMapObject->GetHandle())) {
                hitMapObject->TakeDamage(projectile.GetDamage());
                projectile.RecordHitMapObject(hitMapObject->GetHandle());
                hitEntity = true;
            }

            if (hitWall && !projectile.IsEnemyProjectile()) {
                if (effectManager) {
                    effectManager->AddImpactEffect({
                        projectileBox.x + projectileBox.width * 0.5f,
                        projectileBox.y + projectileBox.height * 0.5f
                    });
                }
                if (effectManager) effectManager->SpawnImpact(
                    {
                        projectileBox.x + projectileBox.width * 0.5f,
                        projectileBox.y + projectileBox.height * 0.5f
                    },
                    projectile.GetVelocity(),
                    YELLOW,
                    8
                );
            }
        }

        if (!projectile.IsEnemyProjectile()) {
            for (const std::unique_ptr<Enemy>& enemy : enemies) {
                if (!enemy || !enemy->IsEnabled() || enemy->IsDead()) continue;
                if (!CheckCollisionRecs(
                        projectileBox,
                        enemy->GetBoundingBox()) ||
                    projectile.HasHitTarget(enemy.get())) {
                    continue;
                }

                enemy->TakeDamage(projectile.GetDamage());
                projectile.RecordHit(enemy.get());
                Vector2 knockbackDirection = Vector2Subtract(
                    enemy->GetPosition(),
                    projectile.GetPosition()
                );
                if (Vector2Length(knockbackDirection) > 0.0f) {
                    knockbackDirection = Vector2Normalize(knockbackDirection);
                } else {
                    knockbackDirection = { 1.0f, 0.0f };
                }
                enemy->ApplyKnockback(knockbackDirection, 350.0f);

                Vector2 impactPosition = {
                    projectileBox.x + projectileBox.width * 0.5f,
                    projectileBox.y + projectileBox.height * 0.5f
                };
                if (effectManager) effectManager->AddImpactEffect(impactPosition);
                if (effectManager) effectManager->SpawnImpact(
                    impactPosition,
                    projectile.GetVelocity(),
                    SKYBLUE,
                    6
                );
                if (teamManager && teamManager->GetActivePaladin()) {
                    teamManager->GetActivePaladin()->OnHitEnemy(
                        projectile.GetDamage()
                    );
                }
                hitEntity = true;
                if (!projectile.IsPiercing()) break;
            }
        }

        Paladin* activePaladin = teamManager
            ? teamManager->GetActivePaladin()
            : nullptr;
        if (projectile.IsEnemyProjectile() && activePaladin &&
            CheckCollisionRecs(
                projectileBox,
                activePaladin->GetBoundingBox()) &&
            !projectile.HasHitTarget(activePaladin)) {
            if (activePaladin->CanParryAttack(projectile.GetPosition())) {
                activePaladin->TriggerParrySuccess(&projectile);
                activePaladin->IncrementParryCount();
                if (hitstopCallback) hitstopCallback(0.1f);
                if (effectManager) {
                    effectManager->AddImpactEffect({
                        projectileBox.x + projectileBox.width * 0.5f,
                        projectileBox.y + projectileBox.height * 0.5f
                    });
                }
                if (effectManager) effectManager->SpawnParrySparks(
                    {
                        projectileBox.x + projectileBox.width * 0.5f,
                        projectileBox.y + projectileBox.height * 0.5f
                    },
                    16
                );
            } else {
                activePaladin->TakeDamage(projectile.GetDamage());
                if (activePaladin->IsParrying()) {
                    activePaladin->ChangeState(activePaladin->GetIdleState());
                }
            }
            projectile.RecordHit(activePaladin);
            hitEntity = true;
        }

        if (projectile.IsEnemyProjectile()) {
            for (const std::unique_ptr<Rover>& rover : assists) {
                if (!rover || rover->IsDead()) continue;
                if (!CheckCollisionRecs(
                        projectileBox,
                        rover->GetBoundingBox()) ||
                    projectile.HasHitTarget(rover.get())) {
                    continue;
                }
                rover->TakeDamage(projectile.GetDamage());
                projectile.RecordHit(rover.get());
                if (effectManager) {
                    effectManager->AddImpactEffect({
                        projectileBox.x + projectileBox.width * 0.5f,
                        projectileBox.y + projectileBox.height * 0.5f
                    });
                }
                hitEntity = true;
                if (!projectile.IsPiercing()) break;
            }
        }

        if (hitWall || hitEntity) {
            if (projectile.IsPiercing() &&
                !projectile.IsEnemyProjectile()) {
                if (hitWall) projectile.SetReturning(true);
            } else {
                projectile.Destroy();
            }
        }

        if (!projectile.IsActive()) {
            iterator = projectiles.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

void ObjectManager::AddRover(std::unique_ptr<Rover> rover) {
    if (!rover) return;
    if (!assists.empty()) {
        assists.front()->Heal();
    } else {
        assists.push_back(std::move(rover));
    }
}

void ObjectManager::UpdateAssists(float deltaTime) {
    for (const std::unique_ptr<Rover>& rover : assists) {
        if (rover) rover->Update(deltaTime);
    }
    assists.erase(
        std::remove_if(
            assists.begin(),
            assists.end(),
            [](const std::unique_ptr<Rover>& rover) {
                return !rover || rover->IsDead();
            }
        ),
        assists.end()
    );
}

void ObjectManager::SpawnQuintessenceOrb(Vector2 position) {
    QuintessenceOrb orb;
    orb.position = position;
    orb.velocity = {
        (float)GetRandomValue(-100, 100),
        (float)GetRandomValue(-100, 100)
    };
    orbs.push_back(std::move(orb));
}

void ObjectManager::UpdateOrbs(float deltaTime) {
    Paladin* player = teamManager ? teamManager->GetActivePaladin() : nullptr;
    for (auto iterator = orbs.begin(); iterator != orbs.end();) {
        if (!iterator->isAttracted) {
            iterator->velocity.x -= iterator->velocity.x * 4.0f * deltaTime;
            iterator->velocity.y -= iterator->velocity.y * 4.0f * deltaTime;
        }
        iterator->position = Vector2Add(
            iterator->position,
            Vector2Scale(iterator->velocity, deltaTime)
        );
        iterator->positionHistory.push_front(iterator->position);
        if (iterator->positionHistory.size() > 12) {
            iterator->positionHistory.pop_back();
        }

        if (player) {
            Vector2 difference = Vector2Subtract(
                player->GetPosition(),
                iterator->position
            );
            float distanceSquared = Vector2LengthSqr(difference);
            if (distanceSquared < 75.0f * 75.0f) {
                iterator->isAttracted = true;
            }
            if (iterator->isAttracted && distanceSquared > 0.0f) {
                iterator->velocity = Vector2Scale(
                    Vector2Normalize(difference),
                    400.0f
                );
            }
            if (distanceSquared < 20.0f * 20.0f) {
                teamManager->AddQuintessence(10.0f);
                AudioManager::GetInstance().PlaySoundEffect("fx_energy");
                iterator = orbs.erase(iterator);
                continue;
            }
        }
        ++iterator;
    }
}

void ObjectManager::CommitPendingChanges() {
    ProcessPendingAdditions();
    ProcessPendingRemovals();
}

void ObjectManager::AddDepthRenderItems(
    std::vector<DepthRenderItem>& items
) {
    auto appendObject = [&items](GameObject* object) {
        if (!object) return;
        items.push_back({
            object->GetBoundingBox().y + object->GetBoundingBox().height,
            [object]() { object->Draw(); }
        });
    };
    for (const std::unique_ptr<Enemy>& enemy : enemies) {
        appendObject(enemy.get());
    }
    for (const std::unique_ptr<Pot>& pickup : pickups) {
        appendObject(pickup.get());
    }
    for (const std::unique_ptr<GameObject>& interactable : interactables) {
        appendObject(interactable.get());
    }
    for (const std::unique_ptr<Projectile>& projectile : projectiles) {
        if (!projectile) continue;
        Projectile* pointer = projectile.get();
        items.push_back({
            pointer->GetBoundingBox().y + pointer->GetBoundingBox().height,
            [pointer]() { pointer->Draw(); }
        });
    }
    for (const std::unique_ptr<Rover>& rover : assists) {
        if (!rover) continue;
        Rover* pointer = rover.get();
        items.push_back({
            pointer->GetPosition().y,
            [pointer]() { pointer->Draw(); }
        });
    }
}

void ObjectManager::DrawOrbs() const {
    Texture2D texture = AssetManager::GetInstance().GetTexture("Quint_Orb");
    for (const QuintessenceOrb& orb : orbs) {
        for (std::size_t index = 0;
             index + 1 < orb.positionHistory.size();
             ++index) {
            float progress = 1.0f -
                (float)index / (float)orb.positionHistory.size();
            DrawLineEx(
                orb.positionHistory[index],
                orb.positionHistory[index + 1],
                progress * 6.0f,
                ColorAlpha(SKYBLUE, progress * 0.8f)
            );
        }
        DrawTexturePro(
            texture,
            { 0.0f, 0.0f, (float)texture.width, (float)texture.height },
            {
                orb.position.x,
                orb.position.y,
                (float)texture.width,
                (float)texture.height
            },
            { texture.width * 0.5f, texture.height * 0.5f },
            0.0f,
            WHITE
        );
    }
}

void ObjectManager::DrawDebugOverlays() const {
    if (Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES) {
        for (const std::unique_ptr<Enemy>& enemy : enemies) {
            if (enemy) DrawObjectCollisionDebug(*enemy);
        }
        for (const std::unique_ptr<Pot>& pickup : pickups) {
            if (pickup) DrawObjectCollisionDebug(*pickup);
        }
        for (const std::unique_ptr<GameObject>& object : interactables) {
            if (object) DrawObjectCollisionDebug(*object);
        }
        for (const std::unique_ptr<Projectile>& projectile : projectiles) {
            if (projectile && projectile->IsActive()) {
                DrawObjectCollisionDebug(*projectile);
            }
        }
        for (const std::unique_ptr<Rover>& rover : assists) {
            if (rover && !rover->IsDead()) DrawObjectCollisionDebug(*rover);
        }
    }
    if (Constants::DEBUG_DRAW_ENEMY_PATHS) {
        for (const std::unique_ptr<Enemy>& enemy : enemies) {
            if (enemy) enemy->DrawPathDebug();
        }
    }
}

GameObject* ObjectManager::FindObject(ObjectId id) const {
    if (id == INVALID_OBJECT_ID) return nullptr;
    for (Enemy* enemy : enemyView) {
        if (enemy && enemy->GetObjectId() == id) return enemy;
    }
    for (const std::unique_ptr<Pot>& pickup : pickups) {
        if (pickup && pickup->GetObjectId() == id) return pickup.get();
    }
    for (GameObject* object : interactableView) {
        if (object && object->GetObjectId() == id) return object;
    }
    for (const std::unique_ptr<Projectile>& projectile : projectiles) {
        if (projectile && projectile->GetObjectId() == id) {
            return projectile.get();
        }
    }
    for (const std::unique_ptr<Rover>& rover : assists) {
        if (rover && rover->GetObjectId() == id) return rover.get();
    }
    return nullptr;
}

Enemy* ObjectManager::FindEnemy(ObjectId id) const {
    for (Enemy* enemy : enemyView) {
        if (enemy && enemy->GetObjectId() == id) return enemy;
    }
    return nullptr;
}

Pot* ObjectManager::FindNearestPickup(Vector2 position, float radius) const {
    Pot* nearest = nullptr;
    float nearestDistanceSquared = radius * radius;
    for (const std::unique_ptr<Pot>& pickup : pickups) {
        if (!pickup || pickup->IsConsumed()) continue;
        float distanceSquared = Vector2DistanceSqr(
            position,
            pickup->GetPosition()
        );
        if (distanceSquared < nearestDistanceSquared) {
            nearestDistanceSquared = distanceSquared;
            nearest = pickup.get();
        }
    }
    return nearest;
}

bool ObjectManager::IsDynamicCollisionBlocked(
    Rectangle bounds,
    const GameObject* ignored
) const {
    auto collides = [bounds, ignored](const GameObject* object) {
        return object && object != ignored &&
            CheckCollisionRecs(bounds, object->GetCollisionBox());
    };
    for (Enemy* enemy : enemyView) {
        if (collides(enemy)) return true;
    }
    for (const std::unique_ptr<Pot>& pickup : pickups) {
        if (collides(pickup.get())) return true;
    }
    for (GameObject* object : interactableView) {
        if (collides(object)) return true;
    }
    return false;
}

void ObjectManager::Clear() {
    pendingRemoval.clear();
    pendingAddition.clear();
    finalizedDeaths.clear();
    silentRemoval.clear();
    projectiles.clear();
    assists.clear();
    orbs.clear();
    pickups.clear();
    interactables.clear();
    enemies.clear();
    RebuildViews();
}

void ObjectManager::ClearProjectiles() {
    projectiles.clear();
}

void ObjectManager::ClearOrbs() {
    orbs.clear();
}
