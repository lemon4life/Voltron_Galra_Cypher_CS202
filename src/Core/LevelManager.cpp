#include "Core/LevelManager.h"
#include "Core/EntityFactory.h"
#include <fstream>
#include <iostream>

#include "Entities/Enemy.h"

LevelManager::LevelManager() {}

LevelManager::~LevelManager() {
    ClearLevel();
}

void LevelManager::LoadLevel(const std::string& filepath, Player* player) {
    ClearLevel();

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open level file: " << filepath << std::endl;
        return;
    }

    std::string line;
    int row = 0;
    while (std::getline(file, line)) {
        for (int col = 0; col < line.length(); ++col) {
            char type = line[col];
            // Calculate screen position (32x32 tiles)
            Vector2 position = { (float)col * 32.0f, (float)row * 32.0f };
            
            GameObject* entity = EntityFactory::CreateEntity(type, position, player);
            if (entity != nullptr) {
                levelEntities.push_back(entity);
            }
        }
        row++;
    }
    file.close();
}

void LevelManager::UpdateLevel(float deltaTime) {
    for (auto it = levelEntities.begin(); it != levelEntities.end();) {
        (*it)->Update(deltaTime);
        
        if (Enemy* e = dynamic_cast<Enemy*>(*it)) {
            if (e->IsDead()) {
                delete *it;
                it = levelEntities.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void LevelManager::DrawLevel() {
    for (auto* entity : levelEntities) {
        entity->Draw();
    }
}

void LevelManager::ClearLevel() {
    for (auto* entity : levelEntities) {
        delete entity;
    }
    levelEntities.clear();
}
