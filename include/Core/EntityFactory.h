#pragma once
#include "Entities/GameObject.h"

class Player;

class EntityFactory {
public:
    static GameObject* CreateEntity(char type, Vector2 position, Player* player);
};
