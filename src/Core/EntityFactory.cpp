#include "Core/EntityFactory.h"
#include "Entities/Enemy.h"
#include "Entities/Wall.h"
#include "Entities/NPC.h"
#include "Entities/EnemyEntities/EnemyChaser.h"
#include "Entities/EnemyEntities/Boss.h"
#include "Entities/EnemyEntities/EnemyDiver.h"
#include "Entities/EnemyEntities/EnemyRange.h"

GameObject* EntityFactory::CreateEntity(
    char type,
    Vector2 position,
    TeamManager* teamManager,
    const LevelAccessBundle& levelAccess
) {
    switch (type) {
        case 'W':
            return new Wall(position);
        case 'N':
            return new NPC(position);
        case 'E':
            return new EnemyChaser(
                position,
                teamManager,
                levelAccess.removal,
                levelAccess.pathFinding
            );
        case 'B':
            return new Boss(position, teamManager, levelAccess.removal);
        case 'R':
            return new EnemyRange(
                position,
                teamManager,
                levelAccess.removal,
                levelAccess.pathFinding,
                levelAccess.lineOfSight
            );
        case 'D':
            return new EnemyDiver(
                position,
                teamManager,
                levelAccess.removal,
                levelAccess.pathFinding,
                levelAccess.lineOfSight
            );
        // We will add 'c' for Chest later
        default:
            return nullptr;
    }
}
