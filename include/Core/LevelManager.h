#pragma once
#include <vector>
#include <string>
#include "raylib.h"
#include "Entities/GameObject.h"

class Player;

class LevelManager {
private:
    std::vector<GameObject*> levelEntities;

public:
    LevelManager();
    ~LevelManager();

    void LoadLevel(const std::string& filepath, Player* player);
    void DrawLevel();
    void UpdateLevel(float deltaTime);
    void ClearLevel();
    void AddEntity(GameObject* entity);
    bool IsValidSpawnLocation(Vector2 position) const;
    bool IsSolidCollision(Rectangle box) const;
    
    float GetLevelWidth() const { return levelWidth; }
    float GetLevelHeight() const { return levelHeight; }

    const std::vector<GameObject*>& GetEntities() const { return levelEntities; }

private:
    float levelWidth;
    float levelHeight;
    int gridRows;
    int gridCols;
    std::vector<std::vector<int>> mapGridLayer1;
    std::vector<std::vector<int>> mapGridLayer2;
    Texture2D tileset;
};
