#pragma once
#include <vector>
#include <string>
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

    const std::vector<GameObject*>& GetEntities() const { return levelEntities; }
};
