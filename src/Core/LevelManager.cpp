#include "Core/LevelManager.h"
#include "Core/EntityFactory.h"
#include "Core/GameManager.h"
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
    levelWidth = 0.0f;
    levelHeight = 0.0f;
    while (std::getline(file, line)) {
        float currentRowWidth = line.length() * 32.0f;
        if (currentRowWidth > levelWidth) levelWidth = currentRowWidth;
        
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
    levelHeight = row * 32.0f;
    file.close();
    
    // Store in GameManager for global access
    GameManager::GetInstance().SetLevelBounds(levelWidth, levelHeight);
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

#include "Entities/Wall.h"

void LevelManager::AddEntity(GameObject* entity) {
    if (entity) {
        levelEntities.push_back(entity);
    }
}

bool LevelManager::IsValidSpawnLocation(Vector2 position) const {
    // An enemy's bounding box is roughly 24x36, centered.
    Rectangle spawnBox = { position.x - 12.0f, position.y - 18.0f, 24.0f, 36.0f };
    for (auto* entity : levelEntities) {
        if (dynamic_cast<Wall*>(entity)) {
            if (CheckCollisionRecs(spawnBox, entity->GetBoundingBox())) {
                return false;
            }
        }
    }
    return true;
}
