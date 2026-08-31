#include "Core/Manager/ObjectManager.h"

#include "Core/Constants.h"
#include "Core/EntityFactory.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/EffectManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Level/VisibleWorld.h"
#include "Entities/Enemy.h"
#include "Entities/EnemyEntities/Boss.h"
#include "Entities/GameObject.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Projectile.h"
#include "Entities/Props/Pot.h"
#include "Entities/Props/Chest.h"
#include "Entities/Props/EnhanceMachine.h"
#include "Entities/Rover.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace {
constexpr std::size_t MAX_QUINTESSENCE_ORBS = 256;
constexpr float QUINTESSENCE_ORB_LIFETIME = 30.0f;
constexpr int MAX_SAFE_SPAWN_CANDIDATES = 32;
constexpr float SPAWN_BOUNDS_MARGIN = 0.25f;
constexpr float ENEMY_SPATIAL_CELL_SIZE = 64.0f;

/// Packs a two-dimensional spatial-grid coordinate into one hash key.
std::uint64_t SpatialCellKey(int x, int y) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 32U) |
        static_cast<std::uint32_t>(y);
}

/// Reports whether the inner rectangle fits completely inside the outer rectangle.
bool ContainsRectangle(Rectangle outer, Rectangle inner) {
    return inner.x >= outer.x + SPAWN_BOUNDS_MARGIN &&
        inner.y >= outer.y + SPAWN_BOUNDS_MARGIN &&
        inner.x + inner.width <=
            outer.x + outer.width - SPAWN_BOUNDS_MARGIN &&
        inner.y + inner.height <=
            outer.y + outer.height - SPAWN_BOUNDS_MARGIN;
}

/// Renders object collision debug.
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

/// Creates a ObjectManager instance from the supplied configuration.
ObjectManager::ObjectManager() = default;

/// Releases resources owned by this ObjectManager instance.
ObjectManager::~ObjectManager() {
    Clear();
}

/// Connects this component to the managers and services it needs at runtime.
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

/// Rebuilds views.
void ObjectManager::RebuildViews() {
    enemyView.clear();
    enemyIndex.clear();
    interactableView.clear();
    enemyView.reserve(enemies.size());
    interactableView.reserve(interactables.size());

    for (const std::unique_ptr<Enemy>& enemy : enemies) {
        if (!enemy) continue;
        enemyView.push_back(enemy.get());
        enemyIndex.emplace(enemy->GetObjectId(), enemy.get());
    }
    for (const std::unique_ptr<GameObject>& interactable : interactables) {
        if (!interactable) continue;
        interactableView.push_back(interactable.get());
    }
    RebuildEnemySpatialIndex();
}

/// Rebuilds enemy spatial index.
void ObjectManager::RebuildEnemySpatialIndex() {
    enemySpatialBuckets.clear();
    enemySpatialBuckets.reserve(enemyView.size());
    for (Enemy* enemy : enemyView) {
        if (!enemy || enemy->IsDead() || !enemy->IsEnabled()) continue;
        Vector2 position = enemy->GetPosition();
        int cellX = static_cast<int>(std::floor(
            position.x / ENEMY_SPATIAL_CELL_SIZE
        ));
        int cellY = static_cast<int>(std::floor(
            position.y / ENEMY_SPATIAL_CELL_SIZE
        ));
        enemySpatialBuckets[SpatialCellKey(cellX, cellY)].push_back(enemy);
    }
}

/// Transfers a newly created object into its type-specific ownership container.
/// Runtime views are rebuilt by the caller after batches, avoiding stale raw pointers.
void ObjectManager::RouteObject(std::unique_ptr<GameObject> object) {
    if (!object) return;

    if (dynamic_cast<Enemy*>(object.get())) {
        std::unique_ptr<Enemy> ownedEnemy(
            static_cast<Enemy*>(object.release())
        );
        enemies.push_back(std::move(ownedEnemy));
        return;
    }
    if (dynamic_cast<Pot*>(object.get())) {
        std::unique_ptr<Pot> ownedPickup(
            static_cast<Pot*>(object.release())
        );
        pickups.push_back(std::move(ownedPickup));
        return;
    }
    interactables.push_back(std::move(object));
}

/// Adds object.
void ObjectManager::AddObject(std::unique_ptr<GameObject> object) {
    RouteObject(std::move(object));
    RebuildViews();
}

/// Spawns all.
void ObjectManager::SpawnAll(const DynamicSpawnList& requests) {
    for (const DynamicSpawnRequest& request : requests) {
        Spawn(request.type, request.position);
    }
}

/// Creates and queues the requested runtime entity without mutating active iteration.
GameObject* ObjectManager::Spawn(MapObjectId type, Vector2 position) {
    if (!teamManager || !pathFinding || !levelManager) {
        throw std::logic_error(
            "ObjectManager::Spawn called before manager configuration"
        );
    }
    std::unique_ptr<GameObject> object = EntityFactory::CreateEntity(
        type,
        position,
        teamManager,
        *this,
        *pathFinding,
        *levelManager
    );
    if (!object) {
        throw std::invalid_argument(
            "ObjectManager::Spawn does not support MapObjectId " +
            std::to_string(static_cast<int>(type))
        );
    }
    GameObject* result = object.get();
    RouteObject(std::move(object));
    RebuildViews();
    return result;
}

/// Queues spawn.
bool ObjectManager::QueueSpawn(MapObjectId type, Vector2 position) {
    if (!teamManager || !pathFinding || !levelManager) {
        throw std::logic_error(
            "ObjectManager::QueueSpawn called before manager configuration"
        );
    }

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

/// Searches outward from a requested spawn for a legal enemy footprint.
/// Candidates must remain in the room/map and avoid static geometry, the player,
/// existing entities, and already queued spawns before ownership is queued.
bool ObjectManager::QueueEnemySpawnSafely(
    MapObjectId type,
    Vector2 desiredPosition,
    Rectangle allowedRoomBounds,
    float correctionRadius,
    const std::function<void(Enemy&)>& configureEnemy
) {
    if (!teamManager || !pathFinding || !levelManager ||
        allowedRoomBounds.width <= 0.0f ||
        allowedRoomBounds.height <= 0.0f) {
        return false;
    }

    std::unique_ptr<GameObject> object = EntityFactory::CreateEntity(
        type,
        desiredPosition,
        teamManager,
        *this,
        *pathFinding,
        *levelManager
    );
    Enemy* enemy = dynamic_cast<Enemy*>(object.get());
    if (!enemy) return false;
    if (configureEnemy) configureEnemy(*enemy);

    Rectangle levelBounds = levelManager->GetLevelBounds();
    Paladin* activePlayer = teamManager->GetActivePaladin();
    int testedCandidates = 0;

    auto tryCandidate = [&](Vector2 candidate) {
        if (testedCandidates >= MAX_SAFE_SPAWN_CANDIDATES) return false;
        ++testedCandidates;

        Rectangle footprint = enemy->GetNavigationFootprintAt(candidate);
        if (!ContainsRectangle(allowedRoomBounds, footprint) ||
            !ContainsRectangle(levelBounds, footprint) ||
            levelManager->IsSolidCollision(footprint)) {
            return false;
        }
        if (activePlayer && CheckCollisionRecs(
                footprint,
                activePlayer->GetCollisionBox())) {
            return false;
        }
        if (IsDynamicCollisionBlocked(footprint)) return false;

        for (const std::unique_ptr<GameObject>& pending : pendingAddition) {
            if (pending && CheckCollisionRecs(
                    footprint,
                    pending->GetCollisionBox())) {
                return false;
            }
        }

        enemy->SetPosition(candidate);
        pendingAddition.push_back(std::move(object));
        return true;
    };

    if (tryCandidate(desiredPosition)) return true;

    const EnemyCollisionProfile profile = enemy->GetCollisionProfile();
    const float searchStep = Constants::RENDER_TILE_SIZE * 0.5f;
    Vector2 desiredFootCenter = {
        desiredPosition.x + profile.navigationCenterOffset.x,
        desiredPosition.y + profile.navigationCenterOffset.y
    };
    Vector2 snappedFootCenter = {
        std::round(desiredFootCenter.x / searchStep) * searchStep,
        std::round(desiredFootCenter.y / searchStep) * searchStep
    };
    int maximumRing = std::max(
        0,
        static_cast<int>(std::ceil(
            std::max(0.0f, correctionRadius) / searchStep
        ))
    );

    for (int ring = 0;
         ring <= maximumRing &&
         testedCandidates < MAX_SAFE_SPAWN_CANDIDATES;
         ++ring) {
        for (int offsetY = -ring;
             offsetY <= ring &&
             testedCandidates < MAX_SAFE_SPAWN_CANDIDATES;
             ++offsetY) {
            for (int offsetX = -ring;
                 offsetX <= ring &&
                 testedCandidates < MAX_SAFE_SPAWN_CANDIDATES;
                 ++offsetX) {
                if (ring > 0 &&
                    std::abs(offsetX) != ring &&
                    std::abs(offsetY) != ring) {
                    continue;
                }

                Vector2 candidateFootCenter = {
                    snappedFootCenter.x + offsetX * searchStep,
                    snappedFootCenter.y + offsetY * searchStep
                };
                Vector2 candidate = {
                    candidateFootCenter.x -
                        profile.navigationCenterOffset.x,
                    candidateFootCenter.y -
                        profile.navigationCenterOffset.y
                };
                float deltaX = candidate.x - desiredPosition.x;
                float deltaY = candidate.y - desiredPosition.y;
                if (deltaX * deltaX + deltaY * deltaY >
                    correctionRadius * correctionRadius) {
                    continue;
                }
                if (tryCandidate(candidate)) return true;
            }
        }
    }

    return false;
}

/// Processes pending additions.
void ObjectManager::ProcessPendingAdditions() {
    for (std::unique_ptr<GameObject>& object : pendingAddition) {
        RouteObject(std::move(object));
    }
    pendingAddition.clear();
    RebuildViews();
}

/// Queues removal.
void ObjectManager::QueueRemoval(GameObject* object) {
    if (object) pendingRemoval.insert(object);
}

/// Deletes all enemies.
void ObjectManager::DeleteAllEnemies() {
    for (const std::unique_ptr<Enemy>& enemy : enemies) {
        if (!enemy) continue;
        silentRemoval.insert(enemy->GetObjectId());
        pendingRemoval.insert(enemy.get());
    }
}

/// Completes enemy death rewards and removes the dead enemy from managed views.
void ObjectManager::FinalizeEnemyDeath(Enemy& enemy) {
    ObjectId id = enemy.GetObjectId();
    if (finalizedDeaths.count(id) || silentRemoval.count(id)) return;
    finalizedDeaths.insert(id);

    Texture2D downTexture =
        AssetManager::GetInstance().GetTexture("Enemy_Down");
    Vector2 corpsePosition = enemy.GetPosition();
    if (enemy.GetEnemyType() == EnemyType::BOSS) {
        downTexture = AssetManager::GetInstance().GetTexture("Boss_Down");
        corpsePosition = enemy.GetRenderFootPosition();
    } else if (enemy.GetEnemyType() == EnemyType::DRONE) {
        downTexture = AssetManager::GetInstance().GetTexture("Drone_down");
    } else if (enemy.GetEnemyType() == EnemyType::DEMON_THA) {
        downTexture = AssetManager::GetInstance().GetTexture("THA_Down");
    }
    effectManager->AddCorpse(
        corpsePosition,
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

/// Processes pending removals.
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

/// Advances enemies, pickups, and interactables without changing their vectors.
/// Dead objects are queued for removal and finalized after active iteration ends.
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
    RebuildEnemySpatialIndex();
}

/// Advances the only actor allowed to simulate during a boss cinematic. During
/// the forced spell section, other enemies may finish their cosmetic spawn
/// sequence but cannot enter AI states, move, attack, or pathfind.
void ObjectManager::UpdateBossCinematic(float deltaTime) {
    Boss* cinematicBoss = FindActiveCinematicBoss();
    if (!cinematicBoss) return;

    cinematicBoss->Update(deltaTime);
    bool allowSpawnAnimations =
        cinematicBoss->AllowsCinematicSpawnAnimations();
    if (allowSpawnAnimations) {
        for (const std::unique_ptr<Enemy>& enemy : enemies) {
            if (!enemy || enemy.get() == cinematicBoss ||
                !enemy->IsSpawnSequenceActive()) {
                continue;
            }
            enemy->UpdateSpawnSequence(deltaTime);
        }
    }

    // Boss spells queue summons while enemies are being iterated. Publish them
    // only after the selective update finishes, preserving pointer/view safety.
    ProcessPendingAdditions();
    RebuildEnemySpatialIndex();
}

/// Returns the primary cinematic boss. Clones explicitly opt out so they never
/// take over player input or camera ownership from the real encounter boss.
Boss* ObjectManager::FindActiveCinematicBoss() const {
    for (Enemy* enemy : enemyView) {
        Boss* boss = dynamic_cast<Boss*>(enemy);
        if (boss && !boss->IsClone() && boss->IsCinematicActive() &&
            !boss->IsDead()) {
            return boss;
        }
    }
    return nullptr;
}

/// Adds projectile.
void ObjectManager::AddProjectile(
    std::unique_ptr<Projectile> projectile
) {
    if (projectile) projectiles.push_back(std::move(projectile));
}

/// Advances projectiles, resolves their collisions, and removes inactive shots safely.
/// World collision, swept target hits, parries, and projectile-specific exceptions
/// are resolved here because ObjectManager owns both projectile lifetime and routing.
void ObjectManager::UpdateProjectiles(float deltaTime) {
    if (!levelManager) return;

    for (std::size_t projectileIndex = 0;
         projectileIndex < projectiles.size();) {
        Projectile& projectile = *projectiles[projectileIndex];
        projectile.Update(deltaTime);
        if (!projectile.IsActive()) {
            projectiles[projectileIndex] = std::move(projectiles.back());
            projectiles.pop_back();
            continue;
        }

        Rectangle projectileBox = projectile.GetBoundingBox();
        bool ignoresWorld = projectile.IgnoresWorldCollision();
        MapObject* hitMapObject = nullptr;
        bool hitWall = false;
        bool hitEntity = false;

        if (!ignoresWorld && !projectile.IsReturning()) {
            hitMapObject = levelManager->FindProjectileMapObjectCollision(
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
            constexpr float MAXIMUM_ENEMY_EXTENT = 96.0f;
            Rectangle queryBounds = {
                projectileBox.x - MAXIMUM_ENEMY_EXTENT,
                projectileBox.y - MAXIMUM_ENEMY_EXTENT,
                projectileBox.width + MAXIMUM_ENEMY_EXTENT * 2.0f,
                projectileBox.height + MAXIMUM_ENEMY_EXTENT * 2.0f
            };
            GetEnemiesNear(queryBounds, projectileEnemyScratch);
            for (Enemy* enemy : projectileEnemyScratch) {
                if (!CheckCollisionRecs(
                        projectileBox,
                        enemy->GetBoundingBox()) ||
                    projectile.HasHitTarget(enemy)) {
                    continue;
                }

                enemy->TakeDamage(projectile.GetDamage());
                projectile.RecordHit(enemy);
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
            projectiles[projectileIndex] = std::move(projectiles.back());
            projectiles.pop_back();
        } else {
            ++projectileIndex;
        }
    }
}

/// Adds rover.
void ObjectManager::AddRover(std::unique_ptr<Rover> rover) {
    if (!rover) return;
    if (!assists.empty()) {
        assists.front()->Heal();
    } else {
        assists.push_back(std::move(rover));
    }
}

/// Updates assists.
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

/// Spawns quintessence orb.
void ObjectManager::SpawnQuintessenceOrb(Vector2 position) {
    if (orbs.size() >= MAX_QUINTESSENCE_ORBS) {
        orbs.front() = std::move(orbs.back());
        orbs.pop_back();
    }
    QuintessenceOrb orb;
    orb.position = position;
    orb.velocity = {
        (float)GetRandomValue(-100, 100),
        (float)GetRandomValue(-100, 100)
    };
    orbs.push_back(std::move(orb));
}

/// Updates orbs.
void ObjectManager::UpdateOrbs(float deltaTime) {
    Paladin* player = teamManager ? teamManager->GetActivePaladin() : nullptr;
    std::size_t orbIndex = 0;
    while (orbIndex < orbs.size()) {
        QuintessenceOrb& orb = orbs[orbIndex];
        orb.age += deltaTime;
        if (orb.age >= QUINTESSENCE_ORB_LIFETIME) {
            orb = std::move(orbs.back());
            orbs.pop_back();
            continue;
        }
        if (!orb.isAttracted) {
            orb.velocity.x -= orb.velocity.x * 4.0f * deltaTime;
            orb.velocity.y -= orb.velocity.y * 4.0f * deltaTime;
        }
        orb.position = Vector2Add(
            orb.position,
            Vector2Scale(orb.velocity, deltaTime)
        );
        orb.PushHistory(orb.position);

        if (player) {
            Vector2 difference = Vector2Subtract(
                player->GetPosition(),
                orb.position
            );
            float distanceSquared = Vector2LengthSqr(difference);
            if (distanceSquared < 75.0f * 75.0f) {
                orb.isAttracted = true;
            }
            if (orb.isAttracted && distanceSquared > 0.0f) {
                orb.velocity = Vector2Scale(
                    Vector2Normalize(difference),
                    400.0f
                );
            }
            if (distanceSquared < 20.0f * 20.0f) {
                teamManager->AddQuintessence(10.0f);
                AudioManager::GetInstance().PlaySoundEffect("fx_energy");
                orb = std::move(orbs.back());
                orbs.pop_back();
                continue;
            }
        }
        ++orbIndex;
    }
}

/// Applies queued additions and removals after all entity/projectile loops finish.
/// This is the frame's mutation barrier: views and spatial indexes are rebuilt here.
void ObjectManager::CommitPendingChanges() {
    ProcessPendingAdditions();
    ProcessPendingRemovals();
}

/// Adds depth render items.
void ObjectManager::AddDepthRenderItems(
    std::vector<DepthRenderItem>& items
) {
    auto appendObject = [&items](GameObject* object) {
        if (!object ||
            !IsWorldRectangleVisible(object->GetBoundingBox())) {
            return;
        }
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
        if (!projectile ||
            !IsWorldRectangleVisible(projectile->GetBoundingBox())) {
            continue;
        }
        Projectile* pointer = projectile.get();
        items.push_back({
            pointer->GetBoundingBox().y + pointer->GetBoundingBox().height,
            [pointer]() { pointer->Draw(); }
        });
    }
    for (const std::unique_ptr<Rover>& rover : assists) {
        if (!rover ||
            !IsWorldRectangleVisible(rover->GetBoundingBox())) {
            continue;
        }
        Rover* pointer = rover.get();
        items.push_back({
            pointer->GetPosition().y,
            [pointer]() { pointer->Draw(); }
        });
    }
}

/// Renders orbs.
void ObjectManager::DrawOrbs() const {
    Texture2D texture = AssetManager::GetInstance().GetTexture("Quint_Orb");
    for (const QuintessenceOrb& orb : orbs) {
        for (std::size_t index = 0;
             index + 1 < orb.historyCount;
             ++index) {
            float progress = 1.0f -
                static_cast<float>(index) /
                    static_cast<float>(orb.historyCount);
            DrawLineEx(
                orb.HistoryAt(index),
                orb.HistoryAt(index + 1),
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

/// Renders debug overlays.
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

/// Searches for object.
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

/// Searches for enemy.
Enemy* ObjectManager::FindEnemy(ObjectId id) const {
    auto found = enemyIndex.find(id);
    return found == enemyIndex.end() ? nullptr : found->second;
}

/// Returns the current enemies near.
void ObjectManager::GetEnemiesNear(
    Rectangle bounds,
    std::vector<Enemy*>& output
) const {
    output.clear();
    int minimumX = static_cast<int>(std::floor(
        bounds.x / ENEMY_SPATIAL_CELL_SIZE
    ));
    int maximumX = static_cast<int>(std::floor(
        (bounds.x + bounds.width) / ENEMY_SPATIAL_CELL_SIZE
    ));
    int minimumY = static_cast<int>(std::floor(
        bounds.y / ENEMY_SPATIAL_CELL_SIZE
    ));
    int maximumY = static_cast<int>(std::floor(
        (bounds.y + bounds.height) / ENEMY_SPATIAL_CELL_SIZE
    ));
    for (int y = minimumY; y <= maximumY; ++y) {
        for (int x = minimumX; x <= maximumX; ++x) {
            auto bucket = enemySpatialBuckets.find(SpatialCellKey(x, y));
            if (bucket == enemySpatialBuckets.end()) continue;
            for (Enemy* enemy : bucket->second) {
                if (!enemy || enemy->IsDead() || !enemy->IsEnabled()) {
                    continue;
                }
                Vector2 position = enemy->GetPosition();
                if (position.x >= bounds.x &&
                    position.y >= bounds.y &&
                    position.x <= bounds.x + bounds.width &&
                    position.y <= bounds.y + bounds.height) {
                    output.push_back(enemy);
                }
            }
        }
    }
}

/// Searches for nearest pickup.
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

/// Reports whether the dynamic collision blocked condition is satisfied.
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

/// Removes all runtime entries owned by this component and resets transient state.
void ObjectManager::Clear() {
    decltype(pendingRemoval){}.swap(pendingRemoval);
    decltype(pendingAddition){}.swap(pendingAddition);
    decltype(finalizedDeaths){}.swap(finalizedDeaths);
    decltype(silentRemoval){}.swap(silentRemoval);
    decltype(projectiles){}.swap(projectiles);
    decltype(assists){}.swap(assists);
    decltype(orbs){}.swap(orbs);
    decltype(pickups){}.swap(pickups);
    decltype(interactables){}.swap(interactables);
    decltype(enemies){}.swap(enemies);
    RebuildViews();
}

/// Clears projectiles.
void ObjectManager::ClearProjectiles() {
    decltype(projectiles){}.swap(projectiles);
}

/// Clears orbs.
void ObjectManager::ClearOrbs() {
    decltype(orbs){}.swap(orbs);
}

/// Returns the current memory stats.
ObjectManagerMemoryStats ObjectManager::GetMemoryStats() const {
    ObjectManagerMemoryStats stats;
    stats.enemies = enemies.size();
    stats.enemyCapacity = enemies.capacity();
    stats.projectiles = projectiles.size();
    stats.projectileCapacity = projectiles.capacity();
    stats.pickups = pickups.size();
    stats.assists = assists.size();
    stats.interactables = interactables.size();
    stats.orbs = orbs.size();
    stats.orbCapacity = orbs.capacity();
    stats.pendingAdditions = pendingAddition.size();
    stats.pendingRemovals = pendingRemoval.size();
    return stats;
}

/// Captures unopened utility rewards and machines without touching combat state.
std::vector<SavedDynamicObject> ObjectManager::CaptureCheckpointObjects() const {
    std::vector<SavedDynamicObject> saved;
    saved.reserve(pickups.size() + interactables.size());
    for (const std::unique_ptr<Pot>& pickup : pickups) {
        if (!pickup || pickup->IsConsumed()) continue;
        MapObjectId type = MapObjectId::PotHP;
        if (dynamic_cast<const ExPot*>(pickup.get())) {
            type = MapObjectId::PotEX;
        } else if (dynamic_cast<const QuintPot*>(pickup.get())) {
            type = MapObjectId::PotQuint;
        }
        saved.push_back({ static_cast<int>(type), pickup->GetPosition() });
    }
    for (const std::unique_ptr<GameObject>& object : interactables) {
        if (!object) continue;
        if (const Chest* chest = dynamic_cast<const Chest*>(object.get())) {
            if (!chest->IsOpened()) {
                saved.push_back({
                    static_cast<int>(MapObjectId::Chest),
                    chest->GetPosition()
                });
            }
        } else if (dynamic_cast<const EnhanceMachine*>(object.get())) {
            saved.push_back({
                static_cast<int>(MapObjectId::EnhanceMachine),
                object->GetPosition()
            });
        }
    }
    return saved;
}

/// Drops all transient battle ownership and recreates only checkpoint utilities.
bool ObjectManager::RestoreCheckpointObjects(
    const std::vector<SavedDynamicObject>& savedObjects
) {
    constexpr std::size_t MAX_CHECKPOINT_OBJECTS = 128;
    if (savedObjects.size() > MAX_CHECKPOINT_OBJECTS) return false;
    Clear();
    for (const SavedDynamicObject& saved : savedObjects) {
        MapObjectId type = static_cast<MapObjectId>(saved.type);
        if (type != MapObjectId::Chest &&
            type != MapObjectId::EnhanceMachine &&
            type != MapObjectId::PotHP && type != MapObjectId::PotEX &&
            type != MapObjectId::PotQuint) {
            return false;
        }
        if (!QueueSpawn(type, saved.position)) return false;
    }
    CommitPendingChanges();
    return true;
}
