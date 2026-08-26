#include "Core/EntityFactory.h"
#include "Entities/Enemy.h"
#include "Entities/NPC.h"
#include "Entities/Hub/HubPaladinStand.h"
#include "Entities/Props/Pot.h"
#include "Entities/Props/Chest.h"
#include "Entities/EnemyEntities/EnemyChaser.h"
#include "Entities/EnemyEntities/Boss.h"
#include "Entities/EnemyEntities/EnemyDiver.h"
#include "Entities/EnemyEntities/EnemyRange.h"
#include "Entities/EnemyEntities/Drone.h"
#include "Entities/EnemyEntities/DemonTHA.h"
#include "Core/Manager/AssetManager.h"

namespace {
std::unique_ptr<GameObject> CreateHubPaladinStand(
    PaladinId id,
    Vector2 position
) {
    const PaladinDefinition& definition = PaladinCatalog::Get(id);
    return std::make_unique<HubPaladinStand>(
        id,
        position,
        AssetManager::GetInstance().GetTexture(definition.idleTextureKey)
    );
}

std::unique_ptr<GameObject> PrepareEnemySpawn(
    std::unique_ptr<Enemy> enemy
) {
    if (enemy) {
        enemy->BeginSpawnSequence();
    }
    return enemy;
}
}

std::unique_ptr<GameObject> EntityFactory::CreateEntity(
    MapObjectId type,
    Vector2 position,
    TeamManager* teamManager,
    IEntityRemovalAccess& removalAccess,
    IEnemyPathAccess& pathAccess,
    ILevelLineOfSightQuery& lineOfSight
) {
    switch (type) {
        case MapObjectId::Chest:
            return std::make_unique<Chest>(position);
        case MapObjectId::PotEX:
            return std::make_unique<ExPot>(position);
        case MapObjectId::PotHP:
            return std::make_unique<HpPot>(position);
        case MapObjectId::PotQuint:
            return std::make_unique<QuintPot>(position);
        case MapObjectId::NPC:
            return std::make_unique<NPC>(position, NpcId::Allura);
        case MapObjectId::ShiroNPC:
            return std::make_unique<NPC>(position, NpcId::Shiro);
        case MapObjectId::Chaser:
            return PrepareEnemySpawn(std::make_unique<EnemyChaser>(
                position,
                teamManager,
                removalAccess,
                pathAccess
            ));
        case MapObjectId::Boss:
            return PrepareEnemySpawn(std::make_unique<Boss>(
                position,
                teamManager,
                removalAccess,
                pathAccess
            ));
        case MapObjectId::Range:
            return PrepareEnemySpawn(std::make_unique<EnemyRange>(
                position,
                teamManager,
                removalAccess,
                pathAccess,
                lineOfSight
            ));
        case MapObjectId::Drone:
            return PrepareEnemySpawn(std::make_unique<Drone>(
                position,
                teamManager,
                removalAccess,
                pathAccess,
                lineOfSight
            ));
        case MapObjectId::Diver:
            return PrepareEnemySpawn(std::make_unique<EnemyDiver>(
                position,
                teamManager,
                removalAccess,
                pathAccess,
                lineOfSight
            ));
        case MapObjectId::DemonTHA:
            return PrepareEnemySpawn(std::make_unique<DemonTHA>(
                position,
                teamManager,
                removalAccess,
                pathAccess,
                lineOfSight
            ));
        case MapObjectId::HubLanceStand:
            return CreateHubPaladinStand(PaladinId::Lance, position);
        case MapObjectId::HubKeithStand:
            return CreateHubPaladinStand(PaladinId::Keith, position);
        case MapObjectId::HubHunkStand:
            return CreateHubPaladinStand(PaladinId::Hunk, position);
        case MapObjectId::HubPidgeStand:
            return CreateHubPaladinStand(PaladinId::Pidge, position);
        case MapObjectId::DestructibleBox:
        case MapObjectId::Prop1:
        case MapObjectId::Prop2:
        case MapObjectId::MockWall:
        case MapObjectId::Empty:
        default:
            return nullptr;
    }
}
