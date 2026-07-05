#include "Core/LevelManager.h"
#include "Core/EntityFactory.h"
#include "Core/GameManager.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>

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

        // Read level from external file and store it 
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        levelGrid.push_back(line);


        float currentRowWidth = line.length() * 32.0f;
        if (currentRowWidth > levelWidth) levelWidth = currentRowWidth;
        
        for (int col = 0; col < line.length(); ++col) {
            char type = line[col];
            // Calculate screen position (32x32 tiles)
            Vector2 position = { (float)col * 32.0f, (float)row * 32.0f };
            
            GameObject* entity = EntityFactory::CreateEntity(type, position, player);
            if (entity != nullptr) {
                AddEntity(entity);
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
    ProcessPendingRemovals();

    enemyPathManager.Update(this, deltaTime);
    
    for (auto* entity : levelEntities) {
        if (Enemy* enemy = dynamic_cast<Enemy*>(entity)) {
            //std::cout << "Update " << enemy->IsDead() << std::endl;
            if (enemy->IsDead()) {
                continue;
            }
        }
        entity->Update(deltaTime);
    }

    ProcessPendingRemovals();
}

void LevelManager::ProcessPendingRemovals() {
    for (Enemy* enemy : pendingRemoval) {
        enemyPathManager.RemoveEnemy(enemy);

        auto it = std::find(levelEntities.begin(), levelEntities.end(), enemy);
        if (it != levelEntities.end()) {
            enemy->RemoveObserver(this);
            delete *it;
            levelEntities.erase(it);
        }
    }
    pendingRemoval.clear();
}

void LevelManager::DrawLevel() {
    for (auto* entity : levelEntities) {
        entity->Draw();
    }
}

void LevelManager::ClearLevel() {
    for (auto* entity : levelEntities) {
        if (Enemy* enemy = dynamic_cast<Enemy*>(entity)) {
            enemyPathManager.RemoveEnemy(enemy);
            enemy->RemoveObserver(this);
        }
        delete entity;
    }
    levelEntities.clear();
    levelGrid.clear();
    pendingRemoval.clear();
}

#include "Entities/Wall.h"

void LevelManager::AddEntity(GameObject* entity) {
    if (entity) {
        if (Enemy* enemy = dynamic_cast<Enemy*>(entity)) {
            enemy->AddObserver(this);
        }
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

char LevelManager::GetTile(int x, int y) const {
    if (y < 0 || y >= (int)levelGrid.size()) return '\0';
    if (x < 0 || x >= (int)levelGrid[y].size()) return '\0';

    return levelGrid[y][x];
}

bool LevelManager::IsWalkableTile(int x, int y) const {
    char tile = GetTile(x, y);

    return tile != '\0' && tile != 'W';
}

Vector2 LevelManager::WorldToTile(Vector2 worldPos) const {
    return {
        std::floor(worldPos.x / 32.0f),
        std::floor(worldPos.y / 32.0f)
    };
}

Vector2 LevelManager::TileToWorld(int tileX, int tileY) const {
    return {
        tileX * 32.0f + 16.0f,
        tileY * 32.0f + 16.0f
    };
}

void LevelManager::OnEnemyPathFind(Enemy* enemy) {
    enemyPathManager.AddEnemy(enemy);
}

void LevelManager::OnEnemyPathFindEnded(Enemy* enemy) {
    enemyPathManager.RemoveEnemy(enemy);
}

void LevelManager::OnEnemyDied(Enemy* enemy) {
    if (!enemy) return;

    if (std::find(pendingRemoval.begin(), pendingRemoval.end(), enemy) == pendingRemoval.end()) {
        pendingRemoval.push_back(enemy);
    }
}
