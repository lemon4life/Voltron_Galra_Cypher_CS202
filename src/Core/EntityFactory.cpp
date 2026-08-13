#include "Core/EntityFactory.h"
#include "Entities/Enemy.h"
#include "Entities/Wall.h"
#include "Entities/NPC.h"
#include "Entities/Hub/HubPaladinStand.h"
#include "Entities/Props/Prop.h"
#include "Entities/EnemyEntities/EnemyChaser.h"
#include "Entities/EnemyEntities/Boss.h"
#include "Entities/EnemyEntities/EnemyDiver.h"
#include "Entities/EnemyEntities/EnemyRange.h"
#include "Core/Manager/AssetManager.h"

namespace {
GameObject* CreateHubPaladinStand(PaladinId id, Vector2 position) {
    const PaladinDefinition& definition = PaladinCatalog::Get(id);
    return new HubPaladinStand(
        id,
        position,
        AssetManager::GetInstance().GetTexture(definition.idleTextureKey)
    );
}

Enemy* PrepareEnemySpawn(Enemy* enemy) {
    if (enemy) {
        enemy->BeginSpawnSequence();
    }
    return enemy;
}
}

GameObject* EntityFactory::CreateEntity(
    MapObjectId type,
    Vector2 position,
    GameObjectCell cell,
    TeamManager* teamManager,
    const LevelAccessBundle& levelAccess
) {
    switch (type) {
        case MapObjectId::DestructibleBox:
        case MapObjectId::Prop1:
        case MapObjectId::Prop2:
        case MapObjectId::MockWall:
            return new Prop(
                position,
                cell,
                levelAccess.mapObjectDestruction,
                type
            );
        case MapObjectId::NPC:
            return new NPC(position);
        case MapObjectId::Chaser:
            return PrepareEnemySpawn(new EnemyChaser(
                position,
                teamManager,
                levelAccess.removal,
                levelAccess.pathFinding
            ));
        case MapObjectId::Boss:
            return PrepareEnemySpawn(new Boss(
                position,
                teamManager,
                levelAccess.removal,
                levelAccess.pathFinding
            ));
        case MapObjectId::Range:
            return PrepareEnemySpawn(new EnemyRange(
                position,
                teamManager,
                levelAccess.removal,
                levelAccess.pathFinding,
                levelAccess.lineOfSight
            ));
        case MapObjectId::Diver:
            return PrepareEnemySpawn(new EnemyDiver(
                position,
                teamManager,
                levelAccess.removal,
                levelAccess.pathFinding,
                levelAccess.lineOfSight
            ));
        case MapObjectId::HubLanceStand:
            return CreateHubPaladinStand(PaladinId::Lance, position);
        case MapObjectId::HubKeithStand:
            return CreateHubPaladinStand(PaladinId::Keith, position);
        case MapObjectId::HubHunkStand:
            return CreateHubPaladinStand(PaladinId::Hunk, position);
        case MapObjectId::HubPidgeStand:
            return CreateHubPaladinStand(PaladinId::Pidge, position);
        case MapObjectId::Empty:
        default:
            return nullptr;
    }
}
