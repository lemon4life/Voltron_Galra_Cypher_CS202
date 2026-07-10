#include "Core/EntityFactory.h"
#include "Entities/Enemy.h"
#include "Entities/Wall.h"
#include "Entities/NPC.h"
#include "Entities/EnemyEntities/Chaser.h"
#include "Entities/EnemyEntities/Boss.h"

GameObject* EntityFactory::CreateEntity(char type, Vector2 position, TeamManager* teamManager) {
    switch (type) {
        case 'W':
            return new Wall(position);
        case 'N':
            return new NPC(position);
        case 'E':
            return new EnemyChaser(position, teamManager);
        case 'B':
            return new Boss(position, teamManager);
        // We will add 'c' for Chest later
        default:
            return nullptr;
    }
}
