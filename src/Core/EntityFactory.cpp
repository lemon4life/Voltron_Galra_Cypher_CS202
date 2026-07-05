#include "Core/EntityFactory.h"
#include "Entities/Wall.h"
#include "Entities/Enemy.h"
#include "Entities/NPC.h"
#include "Entities/EnemyEntities/Chaser.h"

GameObject* EntityFactory::CreateEntity(char type, Vector2 position, Player* player) {
    switch (type) {
        case 'W':
            return new Wall(position);
        case 'E':
            return new Enemy(position, player);
        case 'N':
            return new NPC(position);
        case 'C':
            return new EnemyChaser(position, player);
        // We will add 'c' for Chest later
        default:
            return nullptr;
    }
}
