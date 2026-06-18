#include "Core/EntityFactory.h"
#include "Entities/Wall.h"
#include "Entities/Enemy.h"

GameObject* EntityFactory::CreateEntity(char type, Vector2 position, Player* player) {
    switch (type) {
        case 'W':
            return new Wall(position);
        case 'E':
            return new Enemy(position, player);
        // We will add 'C' for Chest later
        default:
            return nullptr;
    }
}
