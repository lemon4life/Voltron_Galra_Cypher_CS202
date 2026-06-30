#include "Core/EntityFactory.h"
#include "Entities/Enemy.h"
#include "Entities/NPC.h"

GameObject* EntityFactory::CreateEntity(char type, Vector2 position, Player* player) {
    switch (type) {
        case 'E':
            return new Enemy(position, player);
        case 'B': {
            Enemy* boss = new Enemy(position, player);
            boss->SetType(EnemyType::BOSS);
            boss->SetMaxHealth(500);
            boss->SetSpeed(40.0f);
            return boss;
        }
        case 'N':
            return new NPC(position);
        // We will add 'C' for Chest later
        default:
            return nullptr;
    }
}
