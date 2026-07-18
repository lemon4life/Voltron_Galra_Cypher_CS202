#include "Core/EntityFactory.h"
#include "Entities/Enemy.h"
#include "Entities/Wall.h"
#include "Entities/NPC.h"
#include "Entities/EnemyEntities/Chaser.h"
#include "Entities/EnemyEntities/Boss.h"
#include "Entities/EnemyEntities/EnemyDiver.h"
#include "Entities/EnemyEntities/EnemyRange.h"

GameObject* EntityFactory::CreateEntity(char type, Vector2 position, Player* player) {
    switch (type) {
        case 'W':
            return new Wall(position);
        case 'N':
            return new NPC(position);
        case 'E':
            return new EnemyChaser(position, player);
        case 'B':
            return new Boss(position, player);
        case 'R':
            return new EnemyRange(position, player);
        case 'D':
            return new EnemyDiver(position, player);
        // We will add 'c' for Chest later
        default:
            return nullptr;
    }
}
